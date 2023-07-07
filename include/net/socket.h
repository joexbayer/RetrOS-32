#ifndef SOCKET_H
#define SOCKET_H

struct sock;

#include <util.h>
#include <errors.h>
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

    struct ring_buffer* recv_buffer;
	signal_value_t data_ready;
	uint32_t recvd;

    struct sockaddr_in recv_addr;

    /* if tcp socket */
    struct tcp_connection* tcp;
};

#include <net/tcp.h>

#define NET_NUMBER_OF_SOCKETS 128
#define NET_DYNAMIC_PORT_START 49152
#define NET_NUMBER_OF_DYMANIC_PORTS 16383

#define NET_MAX_BUFFER_SIZE 4096*4

void net_sock_bind(struct sock* socket, unsigned short port, unsigned int ip);
int net_sock_read_skb(struct sock* socket);

void init_sockets();
int get_total_sockets();

error_t net_sock_is_established(struct sock* sk);
error_t net_sock_awaiting_ack(struct sock* sk);
error_t net_sock_data_ready(struct sock* sk, unsigned int length);
error_t net_sock_add_data(struct sock* sock, struct sk_buff* skb);

struct sock* sock_get(socket_t id);

error_t net_sock_read(struct sock* sock, uint8_t* buffer, unsigned int length);

struct sock* sock_find_listen_tcp(uint16_t d_port);

struct sock* net_sock_find_tcp(uint16_t s_port, uint16_t d_port, uint32_t ip);
struct sock* net_socket_find_udp(uint32_t ip, uint16_t port);


#endif /* SOCKET_H */
