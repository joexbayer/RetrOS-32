#ifndef __NET_H
#define __NET_H

#include <net/skb.h>
#include <net/interface.h>
#include <net/socket.h>

#define LOOPBACK_IP 0x7f000001
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

error_t kernel_bind(struct sock* socket, const struct sockaddr *address, socklen_t address_len);
error_t kernel_accept(struct sock* socket, struct sockaddr *address, socklen_t *address_len);
error_t kernel_connect(struct sock* socket, const struct sockaddr *address, socklen_t address_len);
error_t kernel_listen(struct sock* socket, int backlog);
error_t kernel_recv(struct sock* socket, void *buffer, int length, int flags);
error_t kernel_recvfrom(struct sock* socket, void *buffer, int length, int flags, struct sockaddr *address, socklen_t *address_len);
error_t kernel_recv_timeout(struct sock* socket, void *buffer, int length, int flags, int timeout);
error_t kernel_send(struct sock* socket, void *message, int length, int flags);
error_t kernel_sendto(struct sock* socket, const void *message, int length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
struct sock* kernel_socket(int domain, int type, int protocol);
void kernel_sock_close(struct sock* socket);

#endif /* __NET_H */
