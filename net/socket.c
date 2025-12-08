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
#include <libc.h>
#include <timer.h>
#include <assert.h>
#include <scheduler.h>
#include <errors.h>

#include <serial.h>

static struct sock** socket_table;
static int total_sockets;
static bitmap_t port_map;
static bitmap_t socket_map;

static spinlock_t __sock_lock = 0;

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
    /* Store bound ports in network order internally. */
    return htons(get_free_bitmap(port_map, NET_NUMBER_OF_DYMANIC_PORTS) + NET_DYNAMIC_PORT_START);
}

void net_sock_bind(struct sock* socket, unsigned short port, unsigned int ip)
{
    socket->bound_ip = ip;
    socket->bound_port = port == 0 ? __get_free_port() : port;
}

/* Currently deprecated */
static int __sock_add_skb(struct sock* socket, struct sk_buff* skb) __attribute__((unused));
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
        sock->waiting = $process->current;
        $process->current->state = BLOCKED;
	    kernel_yield();
    }

    /* If the peer closed the connection but we still have buffered data, return it
     * before reporting EOF. */
    if(sock->data_ready == -1 && sock->recvd == 0){
        dbgprintf(" [SOCK] Socket closed!\n");
        return 0;
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
        
        if(sock->recvd == 0 && sock->data_ready != -1)
            sock->data_ready = 0;

        dbgprintf("[SOCK] Received %d from socket %d\n", to_read, sock);
    });
  
	return to_read;
}

struct sock* sock_get(socket_t id)
{
    if(id >= NET_NUMBER_OF_SOCKETS)
        return NULL;

    struct sock* sock = socket_table[id];
    if(sock != NULL){
        sock_ref(sock);
    }
    return sock;
}

void sock_ref(struct sock* sock)
{
    if(sock == NULL) return;
    __sync_add_and_fetch(&sock->refcount, 1);
}

static void sock_destroy(struct sock* socket)
{
    if(socket == NULL) return;

    int sock_id = socket->socket;
    dbgprintf("[SOCK] Destroying socket %d\n", sock_id);

    tcp_free_connection(socket);
    
    /* Free pending connections list if it exists */
    if(socket->pending_connections != NULL){
        tcp_pending_list_destroy(socket->pending_connections);
        socket->pending_connections = NULL;
    }

    while(SKB_QUEUE_READY(socket->skb_queue)){
        struct sk_buff* skb = socket->skb_queue->ops->remove(socket->skb_queue);
        skb_free(skb);
    }
    skb_free_queue(socket->skb_queue);

    dbgprintf("[SOCK] Freeing recv buffer for socket %d\n", sock_id);
    rbuffer_free(socket->recv_buffer);

    dbgprintf("[SOCK] Socket %d destroyed\n", sock_id);
    kfree((void*) socket);
}

void sock_deref(struct sock* sock)
{
    if(sock == NULL) return;
    if(__sync_sub_and_fetch(&sock->refcount, 1) == 0){
        sock_destroy(sock);
    }
}

static inline error_t net_sock_add_data_segment(struct sock* sock, struct sk_buff* skb)
{
    ASSERT_LOCKED(sock);

    int ret = sock->recv_buffer->ops->add(sock->recv_buffer, skb->data, skb->data_len);
    if(ret < 0){
        dbgprintf("[TCP] recv ring buffer is full!\n");
        return ret;
    }
    sock->recvd += skb->data_len;
    sock->data_ready = sock->tcp == NULL ? 1 : skb->hdr.tcp->psh;

    if(sock->waiting != NULL && sock->waiting->state == BLOCKED){
        /* Wake the process that is blocked on this socket (accept/recv). */
        volatile struct pcb* pcb = sock->waiting;
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

        /* Add segment to socket, the ret is then based on the SKB passed into this function. */
        ret = net_sock_add_data_segment(sock, skb);

        /* While there is still data waiting to be added and data ready is 0 */
        while(!net_sock_data_ready(sock, NET_MAX_BUFFER_SIZE) && SKB_QUEUE_READY(sock->skb_queue)){
            struct sk_buff* queued_skb = sock->skb_queue->ops->remove(sock->skb_queue);
            if(queued_skb == NULL) break;

            net_sock_add_data_segment(sock, queued_skb);
            
            /* The SKB was added in the step above, which means it was not freed */
            skb_free(queued_skb);
        }

        dbgprintf("[TCP] Added segment to socket %d (ready: %d)\n", sock, sock->data_ready);
    });
    
    /* This ret will be a indication as to if we managed to add the SKB, -1 meaning it was added to a queue. */
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
	/* Unblock as soon as we have any bytes buffered (stream semantics), EOF, or the caller's threshold. */
	return sk->data_ready == 1 || sk->recvd >= length || sk->recvd > 0 || sk->data_ready == -1;
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
    //dbgprintf("[TCP] Looking for socket destintation %d: source %d\n", htons(d_port), htons(s_port));
    struct sock* _sk = NULL; /* save listen socket incase no established connection is found. */

    /* First pass: Look for exact match (established connection with matching 4-tuple) */
    for (int i = 0; i < NET_NUMBER_OF_SOCKETS; i++){
        if(socket_table[i] == NULL || socket_table[i]->tcp == NULL)
            continue;

        /* Match on destination port, source port, and source IP for established connections */
        if(socket_table[i]->bound_port == d_port && 
           socket_table[i]->recv_addr.sin_port == s_port &&
           ntohl(socket_table[i]->recv_addr.sin_addr.s_addr) == ip &&
           (socket_table[i]->tcp->state == TCP_ESTABLISHED || 
            socket_table[i]->tcp->state == TCP_SYN_SENT ||
            socket_table[i]->tcp->state == TCP_WAIT_ACK ||
            socket_table[i]->tcp->state == TCP_CLOSE_WAIT ||
            socket_table[i]->tcp->state == TCP_LAST_ACK ||
           socket_table[i]->tcp->state == TCP_FIN_WAIT ||
            socket_table[i]->tcp->state == TCP_FIN_WAIT_2 ||
            socket_table[i]->tcp->state == TCP_TIME_WAIT)) {
                //dbgprintf("[TCP] Found established socket %d\n", i);
                return socket_table[i];
        }
    }
    
    /* Second pass: Look for listening socket if no established connection found */
    for (int i = 0; i < NET_NUMBER_OF_SOCKETS; i++){
        if(socket_table[i] == NULL || socket_table[i]->tcp == NULL)
            continue;
        
        if(socket_table[i]->bound_port == d_port && 
           (socket_table[i]->tcp->state == TCP_LISTEN || socket_table[i]->tcp->state == TCP_SYN_RCVD)){
            _sk = socket_table[i];
            break; /* Found listening socket, use it */
        }
    }

    if(_sk != NULL){
        //dbgprintf("[TCP] Found listening socket %d\n", _sk->socket);
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
    if(socket == NULL){
        return;
    }

    socket->closing = 1;
    socket->data_ready = -1;

    /* Wake any process blocked on this socket (recv/accept/backlog waits). */
    if(socket->waiting != NULL){
        socket->waiting->state = RUNNING;
        socket->waiting = NULL;
    }

    if(socket->type == SOCK_STREAM && socket->tcp != NULL && socket->tcp->state != TCP_CLOSED){
        tcp_close_connection(socket);
    }
}

void kernel_sock_cleanup(struct sock* socket)
{
    if(socket == NULL) return;

    int sock_id = socket->socket;

    spin_lock(&__sock_lock);

    if(socket_table[sock_id] == socket){
        unset_bitmap(socket_map, sock_id);
        socket_table[sock_id] = NULL;
        total_sockets--;
    }

    spin_unlock(&__sock_lock);

    sock_deref(socket);
}

void kernel_sock_close(struct sock* socket)
{
    dbgprintf("Closing socket...\n");
    if(socket->type == SOCK_STREAM && socket->tcp != NULL){
        /* For a listening socket, there is no peer to FIN. Just mark closed and tear down. */
        if(socket->tcp->state == TCP_LISTEN){
            socket->closing = 1;
            socket->tcp->state = TCP_CLOSED;
            socket->tcp->time_wait_expire = 0;
            socket->data_ready = -1;
            kernel_sock_cleanup(socket);
            return;
        }

        socket->closing = 1;
        kernel_sock_shutdown(socket, 0);
        return;
    }

    kernel_sock_shutdown(socket, 0);
    kernel_sock_cleanup(socket);
}

void net_close_sockets_owned_by(struct pcb* owner)
{
    if(owner == NULL){
        return;
    }

    struct sock* owned[NET_NUMBER_OF_SOCKETS] = {0};
    int owned_count = 0;

    /* Collect sockets under lock to avoid races with creation/destruction. */
    spin_lock(&__sock_lock);
    for(int i = 0; i < NET_NUMBER_OF_SOCKETS && owned_count < NET_NUMBER_OF_SOCKETS; i++){
        struct sock* sock = socket_table[i];
        if(sock == NULL || sock->owner != owner){
            continue;
        }

        sock_ref(sock);
        owned[owned_count++] = sock;
    }
    spin_unlock(&__sock_lock);

    /* Close sockets outside the lock so shutdown can proceed normally. */
    for(int i = 0; i < owned_count; i++){
        kernel_sock_close(owned[i]);
        sock_deref(owned[i]);
    }
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
    spin_lock(&__sock_lock);

    //int current = get_free_bitmap(socket_map, NET_NUMBER_OF_SOCKETS);
    int current = get_free_bitmap(socket_map, NET_NUMBER_OF_SOCKETS);
    if(current == -1){
        warningf("Unable to create socket, no free sockets!\n");
        spin_unlock(&__sock_lock);
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
    socket_table[current]->refcount = 1;
    socket_table[current]->closing = 0;

    socket_table[current]->recv_buffer = rbuffer_new(NET_MAX_BUFFER_SIZE);
    if(socket_table[current]->recv_buffer == NULL){
        warningf("Unable to create socket buffer!\n");
        kfree(socket_table[current]);
        socket_table[current] = NULL;
        unset_bitmap(socket_map, current);

        spin_unlock(&__sock_lock);
        return NULL;
    }

	socket_table[current]->data_ready = 0;
	socket_table[current]->recvd = 0;

    socket_table[current]->skb_queue = skb_new_queue();
    if(socket_table[current]->skb_queue == NULL){
        warningf("Unable to create socket queue!\n");
        rbuffer_free(socket_table[current]->recv_buffer);
        kfree(socket_table[current]);
        socket_table[current] = NULL;
        unset_bitmap(socket_map, current);

        spin_unlock(&__sock_lock);
        return NULL;
    }

    socket_table[current]->waiting = NULL;
    socket_table[current]->accept_sock = NULL;

    socket_table[current]->owner = $process->current;

    mutex_init(&(socket_table[current]->lock));

    total_sockets++;

    dbgprintf("Created new sock %d\n", current);

    spin_unlock(&__sock_lock);

    return socket_table[current];
}

void net_init_sockets()
{
    socket_table = (struct sock**) kalloc(NET_NUMBER_OF_SOCKETS * sizeof(void*));
    port_map = create_bitmap(NET_NUMBER_OF_DYMANIC_PORTS);
    socket_map = create_bitmap(NET_NUMBER_OF_SOCKETS);
    total_sockets = 0;
}
