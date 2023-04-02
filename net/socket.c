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

#include <serial.h>

static struct sock** socket_table;
static int total_sockets;
static bitmap_t port_map;
static bitmap_t socket_map;

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
    return ntohs(get_free_bitmap(port_map, NUMBER_OF_DYMANIC_PORTS) + DYNAMIC_PORT_START);
}

void net_sock_bind(struct sock* socket, unsigned short port, unsigned int ip)
{
    socket->bound_ip = ip;
    socket->bound_port = port == 0 ? __get_free_port() : port;
}


static int __sock_add_skb(struct sock* socket, struct sk_buff* skb)
{
    LOCK(socket, {
        /* The queue itself is already spinlock protected */
        socket->skb_queue->ops->add(socket->skb_queue, skb);
    });
    return 0;
}

int net_sock_read_skb(struct sock* socket, char* buffer)
{
    int read = -1;
    struct sk_buff* skb = socket->skb_queue->ops->remove(socket->skb_queue);
    if(skb == NULL) return read;
    read = skb->data_len;
    memcpy(buffer, skb->data, read);
    skb_free(skb);

    return read;
}


int get_total_sockets()
{
    return total_sockets;
}

int net_sock_is_established(struct sock* sk)
{
    assert(sk->tcp != NULL);
    return sk->tcp->state == TCP_ESTABLISHED;
}

struct sock* sock_find_listen_tcp(uint16_t d_port)
{
    for (int i = 0; i < MAX_NUMBER_OF_SOCKETS; i++){   
        if(socket_table[i] == NULL || socket_table[i]->tcp == NULL)
            continue;

        if(socket_table[i]->bound_port == d_port &&  socket_table[i]->tcp->state == TCP_LISTEN)
            return socket_table[i];
    }

    return NULL;
    
}
struct sock* net_sock_find_tcp(uint16_t s_port, uint16_t d_port, uint32_t ip)
{
    struct sock* _sk = NULL; /* save listen socket incase no established connection is found. */
    for (int i = 0; i < MAX_NUMBER_OF_SOCKETS; i++){
        if(socket_table[i] == NULL || socket_table[i]->tcp == NULL)
            continue;
        
        if(socket_table[i]->bound_port == d_port &&  socket_table[i]->tcp->state == TCP_LISTEN)
            _sk = socket_table[i];

        dbgprintf("dport: %d - %d. sport: %d - %d. IP: %i - %i\n", socket_table[i]->bound_port, d_port, socket_table[i]->recv_addr.sin_port, s_port, socket_table[i]->recv_addr.sin_addr.s_addr, ip);
        if(socket_table[i]->bound_port == d_port &&  socket_table[i]->recv_addr.sin_port == s_port && socket_table[i]->tcp->state != TCP_LISTEN && socket_table[i]->recv_addr.sin_addr.s_addr == ip)
            return socket_table[i];
    }

    return _sk;
}


int net_socket_add_skb(struct sk_buff* skb, unsigned short port) 
{   
    /* Interate over sockets and add packet if socket exists with matching port and IP */
    for (int i = 0; i < total_sockets; i++){
        if(socket_table[i] == NULL)
            continue;

        if(socket_table[i]->bound_port == htons(port) && (socket_table[i]->bound_ip == skb->hdr.ip->daddr || socket_table[i]->bound_ip == INADDR_ANY)) {
            return __sock_add_skb(socket_table[i], skb);
        }
    }
    return -1;
}

void kernel_sock_close(struct sock* socket)
{
    dbgprintf("Closing socket...\n");
    skb_free_queue(socket->skb_queue);
    kfree((void*) socket);
    unset_bitmap(socket_map, (int)socket->socket);
    total_sockets--;
}

/**
 * @brief Creates a socket and allocates a struct sock representation.
 * Needed for the network stack to forward data to correct socket.
 * @param domain The domain of the socket
 * @param type Type of the socket
 * @param protocol Protocol (UDP / TCP)
 * @return socket_t 
 */
struct sock* kernel_socket(int domain, int type, int protocol)
{
    //int current = get_free_bitmap(socket_map, MAX_NUMBER_OF_SOCKETS);
    int current = get_free_bitmap(socket_map, MAX_NUMBER_OF_SOCKETS);

    socket_table[current] = kalloc(sizeof(struct sock)); /* Allocate space for a socket. Needs to be freed. */
    memset(socket_table[current], 0, sizeof(struct sock));
    socket_table[current]->domain = domain;
    socket_table[current]->protocol = protocol;
    socket_table[current]->type = type;
    socket_table[current]->socket = current;
    socket_table[current]->bound_port = 0;

    socket_table[current]->skb_queue = skb_new_queue();

    mutex_init(&(socket_table[current]->lock));

    total_sockets++;

    dbgprintf("Created new sock\n");

    return socket_table[current];
}

void init_sockets()
{
    socket_table = (struct sock**) kalloc(MAX_NUMBER_OF_SOCKETS * sizeof(void*));
    port_map = create_bitmap(NUMBER_OF_DYMANIC_PORTS);
    socket_map = create_bitmap(MAX_NUMBER_OF_SOCKETS);
    total_sockets = 0;
}