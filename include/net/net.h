#ifndef __NET_H
#define __NET_H

#include <net/skb.h>
#include <net/socket.h>

void net_incoming_packet_handler();
void net_send_skb(struct sk_buff* skb);

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
