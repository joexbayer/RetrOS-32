/**
 * @file api.c
 * @author Joe Bayer (joexbayer)
 * @brief Userspace network library.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <net/api.h>
#include <net/socket.h>
#include <net/net.h>
#include <syscalls.h>
#include <syscall_helper.h>
#include <serial.h>
#include <lib/net.h>

#pragma GCC diagnostic ignored "-Wcast-function-type"

/* systemcall layer */
error_t sys_kernel_bind(socket_t socket, const struct sockaddr *address, socklen_t address_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    return kernel_bind(sock, address, address_len);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_BIND, sys_kernel_bind);

error_t sys_kernel_accept(socket_t socket, struct sockaddr *address, socklen_t *address_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    struct sock* new = kernel_accept(sock, address, address_len);
    if(new == NULL)
        return -ERROR_INVALID_SOCKET;

    return new->socket;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_ACCEPT, sys_kernel_accept);

error_t sys_kernel_connect(socket_t socket, const struct sockaddr *address, socklen_t address_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    return kernel_connect(sock, address, address_len);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_CONNECT, sys_kernel_connect);

error_t sys_kernel_listen(socket_t socket, int backlog)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    return kernel_listen(sock, backlog);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_LISTEN, sys_kernel_listen);

error_t sys_kernel_recv(socket_t socket, struct net_buffer *net_buffer)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    return kernel_recv(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_RECV, sys_kernel_recv);

error_t sys_kernel_recvfrom(socket_t socket, struct net_buffer *net_buffer, struct sockaddr *address, socklen_t *address_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    return kernel_recvfrom(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags, address, 0);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_RECVFROM, sys_kernel_recvfrom);

error_t sys_kernel_recv_timeout(socket_t socket, struct net_buffer *net_buffer, int timeout)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    return kernel_recv_timeout(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags, timeout);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_RECV_TIMEOUT, sys_kernel_recv_timeout);

error_t sys_kernel_send(socket_t socket, struct net_buffer *net_buffer)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    return kernel_send(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_SEND, sys_kernel_send);

error_t sys_kernel_sendto(socket_t socket, struct net_buffer *net_buffer, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    return kernel_sendto(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags, dest_addr, 0);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_SENDTO, sys_kernel_sendto);

socket_t sys_socket_create(int domain, int type, int protocol)
{
    struct sock* sock = kernel_socket_create(domain, type, protocol);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;
    return sock->socket;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_SOCKET, sys_socket_create);

void sys_kernel_sock_close(socket_t socket)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL){
        warningf("Invalid socket %d closed\n", socket);
        return;
    }

    kernel_sock_close(sock);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_CLOSE, sys_kernel_sock_close);

#pragma GCC diagnostic pop