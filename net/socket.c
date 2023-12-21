/**
 * @file socket.c
 * @author Joe Bayer (joexbayer)
 * @brief Socket functions for networking.
 * @version 0.1
 * @date 2022-06-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/socket.h>
#include <net/utils.h>
#include <net/skb.h>
#include <memory.h>
#include <bitmap.h>
#include <util.h>
#include <timer.h>
#include <assert.h>
#include <scheduler.h>
#include <errors.h>

#include <serial.h>

static struct sock** socket_table;
static int total_sockets;
static bitmap_t port_map;
static bitmap_t socket_map;

static const char* socket_type_str[] = {
    "SOCK",
    "SOCK_UDP",
    "SOCK_TCP",
    "SOCK_RAW"
};
const char* socket_type_to_str(int type){
    return socket_type_str[type];
}

static const char* socket_domain_str[] = {
    "AF_INET",
    "AF_INET6",
    "AF_UNIX"
};
const char* socket_domain_to_str(int domain){
    return socket_domain_str[domain];
}

static const char* socket_protocol_str[] = {
    "IPPROTO_TCP",
    "IPPROTO_UDP",
    "IPPROTO_ICMP"
};
const char* socket_protocol_to_str(int protocol){
    return socket_protocol_str[protocol];
}

/**
 * Example:
 * 
 * struct sockaddr_in myaddr;
 * int s;
 *
 * myaddr.sin_family = AF_INET;
 * myaddr.sin_port = htons(3490);
 * inet_aton("63.161.169.137", &myaddr.sin_addr.s_addr);
 * 
 * s = socket(PF_INET, SOCK_STREAM, 0);
 * bind(s, (struct sockaddr*)myaddr, sizeof(myaddr));
 * 
 */

inline static unsigned short __get_free_port()
{
    return ntohs(get_free_bitmap(port_map, NET_NUMBER_OF_DYMANIC_PORTS) + NET_DYNAMIC_PORT_START);
}

void net_sock_bind(struct sock* socket, unsigned short port, unsigned int ip)
{
    socket->bound_ip = ip;
    socket->bound_port = port == 0 ? __get_free_port() : port;
}

/* Currently deprecated */
static int __sock_add_skb(struct sock* socket, struct sk_buff* skb)
{
    LOCK(socket, {
        /* The queue itself is already spinlock protected */
        socket->skb_queue->ops->add(socket->skb_queue, skb);
    });
    return ERROR_OK;
}

int net_get_sockets(struct sockets* sockets)
{
    struct sockets _sockets = {
        .sockets = socket_table,
        .total_sockets = NET_NUMBER_OF_SOCKETS 
    };

    *sockets = _sockets;

    return ERROR_OK;
}

error_t net_sock_read(struct sock* sock, uint8_t* buffer, unsigned int length)
{
	dbgprintf(" [SOCK] Waiting for data... %d\n", sock);
	/* Should be blocking */
    while(!net_sock_data_ready(sock, length)){
        sock->waiting = current_running;
        current_running->state = BLOCKED;
	    kernel_yield();
    }
	//WAIT(!net_sock_data_ready(sock, length));
    dbgprintf(" [SOCK] Data ready! %d\n", sock);

    int to_read = -1;

    LOCK(sock, {
        to_read = length > sock->recvd ? sock->recvd : length;
        int ret = sock->recv_buffer->ops->read(sock->recv_buffer, buffer, to_read);
        if(ret != to_read){
            dbgprintf("[SOCK] Read from recv buffer not equal to return value!\n");
            break;
        }
        sock->recvd -= to_read;
        
        if(sock->recvd == 0)
            sock->data_ready = 0;

        dbgprintf("[SOCK] Received %d from socket %d\n", to_read, sock);
    });
  
	return to_read;
}

struct sock* sock_get(socket_t id)
{
    if(id > NET_NUMBER_OF_SOCKETS)
        return NULL;

    return socket_table[id];
}

static inline error_t net_sock_add_data_segment(struct sock* sock, struct sk_buff* skb)
{
    ASSERT_LOCKED(sock);

    int ret = sock->recv_buffer->ops->add(sock->recv_buffer, skb->data, skb->data_len);
    if(ret < 0){
        dbgprintf("[TCP] recv ring buffer is full!\n");
        return -ret;
    }
    sock->recvd += skb->data_len;
    sock->data_ready = sock->tcp == NULL ? 1 : skb->hdr.tcp->psh;

    if(sock->waiting->state == BLOCKED){
        /* need to clear waiting before setting it to run */
        struct pcb* pcb = sock->waiting;
        sock->waiting = NULL;
        pcb->state = RUNNING;
    }

    sock->rx += skb->data_len;

    return ERROR_OK;
}

/**
 * @brief This function adds a new network packet to a socket. 
 * Function to add new data to the sockets ring buffer. Both used for UDP and TCP sockets,
 * important to notice: only 1 "packet" can be in the ring buffer at a time. Meaning
 * either one UDP packet or as many TCP packets till the psh flag is set.
 * The function returns an error code if the operation fails.
 * @param sock A pointer to the socket structure to add the new packet to.
 * @param skb A pointer to the new network packet to be added to the socket's queue.
 * @return An integer error code. Returns -1 if the operation fails, or 0 if successful.
 */
error_t net_sock_add_data(struct sock* sock, struct sk_buff* skb)
{
    int ret = -1;
    LOCK(sock, {
        /* if data is ready to be read, we cant add new data, so instead add the skb to the sockets queue.*/
        if(net_sock_data_ready(sock, NET_MAX_BUFFER_SIZE)){
            sock->skb_queue->ops->add(sock->skb_queue, skb);
            dbgprintf("[%d] Adding SKB to socket queue\n", sock->socket);
            break;
        }

        /* Add segment to socket */
        ret = net_sock_add_data_segment(sock, skb);

        /* While there is still data waiting to be added and data ready is 0 */
        while(!net_sock_data_ready(sock, NET_MAX_BUFFER_SIZE) && SKB_QUEUE_READY(sock->skb_queue)){
            struct sk_buff* queued_skb = sock->skb_queue->ops->remove(sock->skb_queue);
            if(queued_skb == NULL) break;

            net_sock_add_data_segment(sock, queued_skb);
            skb_free(queued_skb);
        }

        dbgprintf("[TCP] Added segment to socket %d (ready: %d)\n", sock, sock->data_ready);
    });
    
	return ret;
}


int get_total_sockets()
{
    return total_sockets;
}

/* Helper functions */
error_t net_sock_is_established(struct sock* sk)
{
    assert(sk->tcp != NULL);
    return sk->tcp->state == TCP_ESTABLISHED;
}

/* returns true if socket is waiting for an ack */
error_t net_sock_awaiting_ack(struct sock* sk)
{
    assert(sk->tcp != NULL);
    return sk->tcp->state == TCP_WAIT_ACK;
}

error_t net_sock_data_ready(struct sock* sk, unsigned int length)
{
    assert(sk != NULL);
	return sk->data_ready == 1 || sk->recvd >= length;
}

struct sock* sock_find_listen_tcp(uint16_t d_port)
{
    for (int i = 0; i < NET_NUMBER_OF_SOCKETS; i++){   
        if(socket_table[i] == NULL || socket_table[i]->tcp == NULL)
            continue;

        if(socket_table[i]->bound_port == d_port &&  socket_table[i]->tcp->state == TCP_LISTEN)
            return socket_table[i];
    }

    return NULL;
}


struct sock* net_sock_find_tcp(uint16_t s_port, uint16_t d_port, uint32_t ip)
{
    dbgprintf("[TCP] Looking for socket destintation %d: source %d\n", htons(d_port), htons(s_port));
    struct sock* _sk = NULL; /* save listen socket incase no established connection is found. */

    for (int i = 0; i < NET_NUMBER_OF_SOCKETS; i++){
        if(socket_table[i] == NULL || socket_table[i]->tcp == NULL)
            continue;
       dbgprintf("[TCP] Checking %d %s: source %d: destination %d (%i %i) %s (seq: %d - ack: %d)\n", i,
            socket_table[i]->owner->name, htons(socket_table[i]->recv_addr.sin_port), htons(socket_table[i]->bound_port), ntohl(socket_table[i]->recv_addr.sin_addr.s_addr), ip, tcp_state_to_str(socket_table[i]->tcp->state),
            socket_table[i]->tcp->sequence, socket_table[i]->tcp->acknowledgement); 
    }
    
    for (int i = 0; i < NET_NUMBER_OF_SOCKETS; i++){
        if(socket_table[i] == NULL || socket_table[i]->tcp == NULL)
            continue;

        
        if(socket_table[i]->bound_port == d_port && (socket_table[i]->tcp->state == TCP_LISTEN || socket_table[i]->tcp->state == TCP_SYN_RCVD)){
            _sk = socket_table[i];
        }

        if(socket_table[i]->bound_port == d_port && socket_table[i]->recv_addr.sin_port == s_port
            && socket_table[i]->tcp->state != TCP_LISTEN
            && socket_table[i]->tcp->state != TCP_SYN_RCVD
            && socket_table[i]->tcp->state != TCP_PREPARE
            && ntohl(socket_table[i]->recv_addr.sin_addr.s_addr) == ip
            //&& (socket_table[i]->tcp->state == TCP_ESTABLISHED || socket_table[i]->tcp->state == TCP_SYN_SENT)
            ){
                dbgprintf("[TCP] Found socket %d\n", i);
                return socket_table[i];
            }
    }

    if(_sk != NULL){
        dbgprintf("[TCP] Found socket %d\n", _sk->socket);
    }
    return _sk;
}

int net_prepare_tcp_sock(struct sock* sock, uint16_t port, struct sockaddr_in* addr)
{
    /* TODO: Should not be INADDR_ANY but the IP parent socket. */
    net_sock_bind(sock, port, INADDR_ANY);

    struct sockaddr_in* sptr = &sock->recv_addr;
    memcpy(sptr, addr, sizeof(struct sockaddr_in));

    tcp_new_connection(sock, addr->sin_port, port);
    if(sock->tcp == NULL){
        dbgprintf("[TCP] Unable to create new connection!\n");
        return -1;
    }
    sock->tcp->state = TCP_PREPARE;

    dbgprintf("[TCP] Preparing socket %d\n", sock->socket);

    return 0;
}

int net_sock_accept(struct sock* sock, struct sock* new)
{
    return tcp_accept_connection(sock, new);
}

struct sock* net_socket_find_udp(uint32_t ip, uint16_t port) 
{   
    /* Interate over sockets and add packet if socket exists with matching port and IP */
    for (int i = 0; i < NET_NUMBER_OF_SOCKETS; i++){
        if(socket_table[i] == NULL)
            continue;

        if(socket_table[i]->bound_port == htons(port) && (socket_table[i]->bound_ip == ip || socket_table[i]->bound_ip == INADDR_ANY)) {
            return socket_table[i];
        }
    }
    return NULL;
}

void kernel_sock_shutdown(struct sock* socket, int how)
{
    if(socket->type == SOCK_STREAM && socket->tcp != NULL && socket->tcp->state != TCP_CLOSED){
        tcp_close_connection(socket);
    }
}

void kernel_sock_cleanup(struct sock* socket)
{
    tcp_free_connection(socket);

    while(SKB_QUEUE_READY(socket->skb_queue)){
        struct sk_buff* skb = socket->skb_queue->ops->remove(socket->skb_queue);
        skb_free(skb);
    }

    skb_free_queue(socket->skb_queue);
    rbuffer_free(socket->recv_buffer);

    kfree((void*) socket);
    unset_bitmap(socket_map, (int)socket->socket);

    socket_table[socket->socket] = NULL;

    total_sockets--;
}

void kernel_sock_close(struct sock* socket)
{
    dbgprintf("Closing socket...\n");

    kernel_sock_shutdown(socket, 0);

    kernel_sock_cleanup(socket);
}

/**
 * @brief Creates a socket and allocates a struct sock representation.
 * Needed for the network stack to forward data to correct socket.
 * @param domain The domain of the socket
 * @param type Type of the socket
 * @param protocol Protocol (UDP / TCP)
 * @return socket_t 
 */
struct sock* kernel_socket_create(int domain, int type, int protocol)
{

    /* Should be a lock? */
    ENTER_CRITICAL();

    //int current = get_free_bitmap(socket_map, NET_NUMBER_OF_SOCKETS);
    int current = get_free_bitmap(socket_map, NET_NUMBER_OF_SOCKETS);
    if(current == -1){
        warningf("Unable to create socket, no free sockets!\n");
        LEAVE_CRITICAL();
        return NULL;
    }


    socket_table[current] = create(struct sock); /* Allocate space for a socket. Needs to be freed. */
    memset(socket_table[current], 0, sizeof(struct sock));

    socket_table[current]->domain = domain;
    socket_table[current]->protocol = protocol;
    socket_table[current]->type = type;
    socket_table[current]->socket = current;
    socket_table[current]->bound_port = 0;
    socket_table[current]->bound_ip = 0;
    socket_table[current]->tcp = NULL;
    socket_table[current]->rx = 0;
    socket_table[current]->tx = 0;

    socket_table[current]->recv_buffer = rbuffer_new(NET_MAX_BUFFER_SIZE);
	socket_table[current]->data_ready = 0;
	socket_table[current]->recvd = 0;

    socket_table[current]->skb_queue = skb_new_queue();

    socket_table[current]->waiting = NULL;
    socket_table[current]->accept_sock = NULL;

    socket_table[current]->owner = current_running;

    mutex_init(&(socket_table[current]->lock));

    total_sockets++;

    dbgprintf("Created new sock %d\n", current);

    LEAVE_CRITICAL();

    return socket_table[current];
}

void init_sockets()
{
    socket_table = (struct sock**) kalloc(NET_NUMBER_OF_SOCKETS * sizeof(void*));
    port_map = create_bitmap(NET_NUMBER_OF_DYMANIC_PORTS);
    socket_map = create_bitmap(NET_NUMBER_OF_SOCKETS);
    total_sockets = 0;
}