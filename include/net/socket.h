#ifndef SOCKET_H
#define SOCKET_H

#include <util.h>
#include <sync.h>
#include <net/tcp.h>

/**
 * @brief Following the Linux and UNIX implementation for learning purposes:
 * https://github.com/torvalds/linux/blob/master/include/linux/socket.h
 * https://pubs.opengroup.org/onlinepubs/7908799/xns/syssocket.h.html
 */

/* Domain */
#define AF_UNIX		1	/* Unix domain sockets 		*/
#define AF_LOCAL	1	/* POSIX name for AF_UNIX	*/
#define AF_INET		2	/* Internet IP Protocol 	*/

/* Protocol */
#define SOCK_DGRAM 1
#define SOCK_STREAM 2

#define INADDR_ANY 1

#define BUFFERS_PER_SOCKET 5

typedef uint8_t socket_t;
typedef uint32_t socklen_t;
typedef uint16_t sa_family_t;

struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};

struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
};

struct sockaddr {
	sa_family_t	sa_family;	/* address family, AF_xxx	*/
	char		sa_data[14];	/* 14 bytes of protocol address	*/
};
struct sock {
    int type;
    int protocol;
    int domain;

    mutex_t sock_lock;

    socket_t socket;

    uint16_t bound_port;
    uint16_t bound_ip;

    struct sockaddr_in recv_addr;

    char* buffers[BUFFERS_PER_SOCKET][2048];
    uint16_t buffer_lens[BUFFERS_PER_SOCKET];
    uint8_t last_read_buffer;
    uint8_t next_write_buffer;

    /* if tcp socket */
    struct tcp_connection tcp_conn;
};

int bind(int socket, const struct sockaddr *address, socklen_t address_len);
int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len);
int listen(int socket, int backlog);
int recv(int socket, void *buffer, int length, int flags);
int recvfrom(int socket, void *buffer, int length, int flags, struct sockaddr *address, socklen_t *address_len);
int recv_timeout(int socket, void *buffer, int length, int flags, int timeout);
int send(int socket, const void *message, int length, int flags);
int sendto(int socket, const void *message, int length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
socket_t socket(int domain, int type, int protocol);
void close(socket_t socket);

void init_sockets();
int udp_deliver_packet(uint32_t ip, uint16_t port, char* buffer, uint16_t len);
int get_total_sockets();

struct sock* sock_find_listen_tcp(uint16_t d_port);
struct sock* sock_find_net_tcp(uint16_t s_port, uint16_t d_port);


#endif /* SOCKET_H */
