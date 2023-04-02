#ifndef __NET_H
#define __NET_H

#include <net/skb.h>
#include <net/socket.h>

void net_incoming_packet_handler();
void net_send_skb(struct sk_buff* skb);

int kernel_bind(struct sock* socket, const struct sockaddr *address, socklen_t address_len);
int kernel_accept(struct sock* socket, struct sockaddr *address, socklen_t *address_len);
int kernel_connect(struct sock* socket, const struct sockaddr *address, socklen_t address_len);
int kernel_listen(struct sock* socket, int backlog);
int kernel_recv(struct sock* socket, void *buffer, int length, int flags);
int kernel_recvfrom(struct sock* socket, void *buffer, int length, int flags, struct sockaddr *address, socklen_t *address_len);
int kernel_recv_timeout(struct sock* socket, void *buffer, int length, int flags, int timeout);
int kernel_send(struct sock* socket, void *message, int length, int flags);
int kernel_sendto(struct sock* socket, const void *message, int length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
struct sock* kernel_socket(int domain, int type, int protocol);
void kernel_sock_close(struct sock* socket);

#endif /* __NET_H */
