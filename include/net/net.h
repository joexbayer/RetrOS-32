#ifndef __NET_H
#define __NET_H

#include <stdint.h>
#include <kutils.h>
#include <net/skb.h>
#include <net/interface.h>
#include <net/socket.h>

#define LOOPBACK_IP 0x7f000001

typedef enum net_connection_states {
    NET_CONN_NEW,
    NET_CONN_IN_PROGRESS,
    NET_CONN_ESTABLISHED,
} net_connection_state_t;

/* prototypes */
struct net;
struct net_connection;

struct network {
    uint32_t root;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t broadcast;
};

struct net_connection_ops {
    int (*init)(struct net* net);
    int (*destroy)(struct net* net);
    int (*send)(struct net* net, struct sk_buff* skb);
    int (*recv)(struct net* net, struct sk_buff* skb);
};

struct net_connection {
    net_connection_state_t state;
    struct net_connection_ops* ops;

    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
};

struct net_ops {
    struct net_connection* (*connect)(struct net* net, ...);
};

struct net {
    struct network network;
    struct net_interface* interface;

    /* connections */
    struct net_connection* connections[32];
    int connection_count;

    struct kref ref;
};

struct net* net_create();
void net_destroy(struct net* net);


/* legacy  */

struct net_info {
    int dropped;
    int sent;
    int recvd;
};
error_t net_get_info(struct net_info* info);

void __callback net_incoming_packet(struct netdev* dev);
int net_register_interface(struct net_interface* interface);
int net_send_skb(struct sk_buff* skb);

int net_configure_iface(char* dev, uint32_t ip, uint32_t netmask, uint32_t gateway);
struct net_interface* net_get_iface(uint32_t ip);
struct net_interface** net_get_interfaces();
/* defined in loopback.c */
int net_init_loopback();

void kernel_sock_cleanup(struct sock* socket);

error_t kernel_bind(struct sock* socket, const struct sockaddr *address, socklen_t address_len);
struct sock* kernel_accept(struct sock* socket, struct sockaddr *address, socklen_t *address_len);
error_t kernel_connect(struct sock* socket, const struct sockaddr *address, socklen_t address_len);
error_t kernel_listen(struct sock* socket, int backlog);
error_t kernel_recv(struct sock* socket, void *buffer, int length, int flags);
error_t kernel_recvfrom(struct sock* socket, void *buffer, int length, int flags, struct sockaddr *address, socklen_t *address_len);
error_t kernel_recv_timeout(struct sock* socket, void *buffer, int length, int flags, int timeout);
error_t kernel_send(struct sock* socket, void *message, int length, int flags);
error_t kernel_sendto(struct sock* socket, const void *message, int length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
struct sock* kernel_socket_create(int domain, int type, int protocol);
void kernel_sock_close(struct sock* socket);

#endif /* __NET_H */
