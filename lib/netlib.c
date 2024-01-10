/**
 * @file netlib.c
 * @author Joe Bayer (joexbayer)
 * @brief Userspace network library.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <syscall_helper.h>
#include <lib/syscall.h>
#include <lib/net.h>
#include <errors.h>
#include <kutils.h>
#include <util.h>


#ifdef __cplusplus
extern "C" {
#endif

void close(int socket)
{
    invoke_syscall(SYSCALL_NET_SOCK_CLOSE, socket, 0, 0);
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
    return invoke_syscall(SYSCALL_NET_SOCK_BIND, socket, (int)address, address_len);
}

int accept(int socket, struct sockaddr *address, socklen_t *address_len)
{
    return invoke_syscall(SYSCALL_NET_SOCK_ACCEPT, socket, (int)address, (int)address_len);
}

int connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
    return invoke_syscall(SYSCALL_NET_SOCK_CONNECT, socket, (int)address, address_len);
}

int listen(int socket, int backlog)
{
    return invoke_syscall(SYSCALL_NET_SOCK_LISTEN, socket, backlog, 0);
}

int recv(int socket, void *buffer, int length, int flags)
{
    
    struct net_buffer net_buffer = {
        .buffer = buffer,
        .length = length,
        .flags = flags
    };
    return invoke_syscall(SYSCALL_NET_SOCK_RECV, socket, (int)&net_buffer, 0);
}

int recvfrom(int socket, void *buffer, int length, int flags, struct sockaddr *address, socklen_t *address_len)
{
    
    
    struct net_buffer net_buffer = {
        .buffer = buffer,
        .length = length,
        .flags = flags
    };

    return invoke_syscall(SYSCALL_NET_SOCK_RECVFROM, socket, (int)&net_buffer, (int)address);
}   

int recv_timeout(int socket, void *buffer, int length, int flags, int timeout)
{
    
    struct net_buffer net_buffer = {
        .buffer = buffer,
        .length = length,
        .flags = flags
    };

    return invoke_syscall(SYSCALL_NET_SOCK_RECV_TIMEOUT, socket, (int)&net_buffer, timeout);
}   

int send(int socket, void *message, int length, int flags)
{
    
    struct net_buffer net_buffer = {
        .buffer = message,
        .length = length,
        .flags = flags
    };
    return invoke_syscall(SYSCALL_NET_SOCK_SEND, socket, (int)&net_buffer, 0);
}   

int sendto(int socket, void *message, int length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    struct net_buffer net_buffer = {
        .buffer = message,
        .length = length,
        .flags = flags
    };   
    return invoke_syscall(SYSCALL_NET_SOCK_SENDTO, socket, (int)&net_buffer, (int)dest_addr);
}

int socket(int domain, int type, int protocol)
{
    return invoke_syscall(SYSCALL_NET_SOCK_SOCKET, domain, type, protocol);
}

int gethostname(char *name)
{
    return invoke_syscall(SYSCALL_NET_DNS_LOOKUP, (int)name, 0, 0);
}

#ifdef __cplusplus
}
#endif