#ifndef __NET_API_H
#define __NET_API_H

#include <net/net.h>
#include <lib/net.h>

error_t sys_kernel_bind(socket_t socket, const struct sockaddr *address, socklen_t address_len);
error_t sys_kernel_accept(socket_t socket, struct sockaddr *address, socklen_t *address_len);
error_t sys_kernel_connect(socket_t socket, const struct sockaddr *address, socklen_t address_len);
error_t sys_kernel_listen(socket_t socket, int backlog);
error_t sys_kernel_recv(socket_t socket, struct net_buffer *net_buffer);
error_t sys_kernel_recvfrom(socket_t socket, struct net_buffer *net_buffer, struct sockaddr *address, socklen_t *address_len);
error_t sys_kernel_recv_timeout(socket_t socket, struct net_buffer *net_buffer, int timeout);
error_t sys_kernel_send(socket_t socket, struct net_buffer *net_buffer);
error_t sys_kernel_sendto(socket_t socket, struct net_buffer *net_buffer, const struct sockaddr *dest_addr, socklen_t dest_len);
socket_t sys_kernel_socket_create(int domain, int type, int protocol);
void sys_kernel_sock_close(socket_t socket);

#endif // !__NET_API_H