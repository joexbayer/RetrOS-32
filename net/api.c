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
#include <memory.h>

#pragma GCC diagnostic ignored "-Wcast-function-type"

static error_t copy_in_sockaddr(struct sockaddr_in* dst, const struct sockaddr *user_addr)
{
    if(user_addr == NULL){
        return -ERROR_NULL_POINTER;
    }

    RETURN_ON_ERR(user_memory_validate(user_addr, sizeof(struct sockaddr_in), 0));
    memcpy(dst, user_addr, sizeof(struct sockaddr_in));
    return ERROR_OK;
}

static error_t copy_out_sockaddr(struct sockaddr *user_addr, const struct sockaddr_in* src)
{
    if(user_addr == NULL){
        return -ERROR_NULL_POINTER;
    }

    RETURN_ON_ERR(user_memory_validate(user_addr, sizeof(struct sockaddr_in), 1));
    memcpy(user_addr, src, sizeof(struct sockaddr_in));
    return ERROR_OK;
}

static error_t validate_net_buffer(struct net_buffer* buffer, int write)
{
    if(buffer == NULL){
        return -ERROR_NULL_POINTER;
    }

    RETURN_ON_ERR(user_memory_validate(buffer, sizeof(struct net_buffer), 0));

    if(buffer->buffer != NULL && buffer->length > 0){
        RETURN_ON_ERR(user_memory_validate(buffer->buffer, buffer->length, write));
    }

    return ERROR_OK;
}

/* systemcall layer */
error_t sys_kernel_bind(socket_t socket, const struct sockaddr *address, socklen_t address_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    struct sockaddr_in kaddr;
    error_t ret = copy_in_sockaddr(&kaddr, address);
    if(ret == ERROR_OK){
        ret = kernel_bind(sock, (const struct sockaddr*)&kaddr, address_len);
    }

    sock_deref(sock);
    return ret;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_BIND, sys_kernel_bind);

error_t sys_kernel_accept(socket_t socket, struct sockaddr *address, socklen_t *address_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    struct sockaddr_in kaddr;
    struct sockaddr* kaddr_ptr = NULL;
    socklen_t klen = sizeof(struct sockaddr_in);
    socklen_t* klen_ptr = NULL;

    if(address != NULL){
        kaddr_ptr = (struct sockaddr*)&kaddr;
    }

    if(address_len != NULL){
        RETURN_ON_ERR(user_memory_validate(address_len, sizeof(socklen_t), 1));
        klen = *address_len;
        klen_ptr = &klen;
    }

    struct sock* new = kernel_accept(sock, kaddr_ptr, klen_ptr);
    sock_deref(sock);
    if(new == NULL)
        return -ERROR_INVALID_SOCKET;

    if(address != NULL){
        RETURN_ON_ERR(copy_out_sockaddr(address, &kaddr));
    }

    if(address_len != NULL){
        *address_len = klen;
    }

    return new->socket;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_ACCEPT, sys_kernel_accept);

error_t sys_kernel_connect(socket_t socket, const struct sockaddr *address, socklen_t address_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    struct sockaddr_in kaddr;
    error_t ret = copy_in_sockaddr(&kaddr, address);
    if(ret == ERROR_OK){
        ret = kernel_connect(sock, (const struct sockaddr*)&kaddr, address_len);
    }

    sock_deref(sock);
    return ret;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_CONNECT, sys_kernel_connect);

error_t sys_kernel_listen(socket_t socket, int backlog)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    error_t ret = kernel_listen(sock, backlog);
    sock_deref(sock);
    return ret;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_LISTEN, sys_kernel_listen);

error_t sys_kernel_recv(socket_t socket, struct net_buffer *net_buffer)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    error_t ret = validate_net_buffer(net_buffer, 1);
    if(ret == ERROR_OK){
        ret = kernel_recv(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags);
    }

    sock_deref(sock);
    return ret;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_RECV, sys_kernel_recv);

error_t sys_kernel_recvfrom(socket_t socket, struct net_buffer *net_buffer, struct sockaddr *address, socklen_t *address_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    error_t ret = validate_net_buffer(net_buffer, 1);

    if(ret == ERROR_OK && address != NULL){
        ret = user_memory_validate(address, sizeof(struct sockaddr_in), 1);
    }

    if(ret == ERROR_OK && address_len != NULL){
        ret = user_memory_validate(address_len, sizeof(socklen_t), 1);
    }

    if(ret == ERROR_OK){
        ret = kernel_recvfrom(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags, address, 0);
    }

    sock_deref(sock);
    return ret;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_RECVFROM, sys_kernel_recvfrom);

error_t sys_kernel_recv_timeout(socket_t socket, struct net_buffer *net_buffer, int timeout)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    error_t ret = validate_net_buffer(net_buffer, 1);
    if(ret == ERROR_OK){
        ret = kernel_recv_timeout(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags, timeout);
    }

    sock_deref(sock);
    return ret;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_RECV_TIMEOUT, sys_kernel_recv_timeout);

error_t sys_kernel_send(socket_t socket, struct net_buffer *net_buffer)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    error_t ret = validate_net_buffer(net_buffer, 0);
    if(ret == ERROR_OK){
        ret = kernel_send(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags);
    }

    sock_deref(sock);
    return ret;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_SEND, sys_kernel_send);

error_t sys_kernel_sendto(socket_t socket, struct net_buffer *net_buffer, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL)
        return -ERROR_INVALID_SOCKET;

    error_t ret = validate_net_buffer(net_buffer, 0);

    struct sockaddr_in kaddr;
    if(ret == ERROR_OK){
        ret = copy_in_sockaddr(&kaddr, dest_addr);
    }

    if(ret == ERROR_OK){
        ret = kernel_sendto(sock, net_buffer->buffer, net_buffer->length, net_buffer->flags, (const struct sockaddr*)&kaddr, dest_len);
    }

    sock_deref(sock);
    return ret;
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

error_t sys_kernel_sock_shutdown(socket_t socket, int how)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL){
        warningf("Invalid socket %d shutdown\n", socket);
        return -ERROR_INVALID_SOCKET;
    }

    kernel_sock_close(sock);
    sock_deref(sock);
    return ERROR_OK;
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_SHUTDOWN, sys_kernel_sock_shutdown);

void sys_kernel_sock_close(socket_t socket)
{
    struct sock* sock = sock_get(socket);
    if(sock == NULL){
        warningf("Invalid socket %d closed\n", socket);
        return;
    }

    kernel_sock_close(sock);
    sock_deref(sock);
}
EXPORT_SYSCALL(SYSCALL_NET_SOCK_CLOSE, sys_kernel_sock_close);

#pragma GCC diagnostic pop
