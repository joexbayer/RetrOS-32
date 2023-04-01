#ifndef SOCKET_H
#define SOCKET_H

#include <util.h>
#include <sync.h>
#include <net/skb.h>
#include <lib/net.h>

/* forward declare struct sock*/
struct sock {
    socket_t socket;

    int type;
    int protocol;
    int domain;

    mutex_t lock;

    uint16_t bound_port;
    uint16_t bound_ip;

    struct skb_queue* skb_queue;
    struct sockaddr_in recv_addr;

    /* if tcp socket */
    struct tcp_connection* tcp;
};

#include <net/tcp.h>

#define MAX_NUMBER_OF_SOCKETS 128
#define DYNAMIC_PORT_START 49152
#define NUMBER_OF_DYMANIC_PORTS 16383

void net_sock_bind(struct sock* socket, unsigned short port, unsigned int ip);
int net_sock_read_skb(struct sock* socket, char* buffer);

void init_sockets();
int net_socket_add_skb(struct sk_buff*,  unsigned short port);
int get_total_sockets();

struct sock* sock_find_listen_tcp(uint16_t d_port);
struct sock* net_sock_find_tcp(uint16_t s_port, uint16_t d_port, uint32_t ip);


#endif /* SOCKET_H */
