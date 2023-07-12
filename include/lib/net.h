#ifndef __LIB_NET_H
#define __LIB_NET_H

#ifdef __cplusplus
extern "C" {
#endif


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

typedef unsigned short socket_t;
typedef unsigned int socklen_t;
typedef unsigned short sa_family_t;

struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};

struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, above
    char             sin_zero[8];  // zero this if you want to
};

struct sockaddr {
	sa_family_t	sa_family;	/* address family, AF_xxx	*/
	char		sa_data[14];	/* 14 bytes of protocol address	*/
};

struct sock_info {
    socket_t sd;
    /* TODO */
};

struct net_buffer {
    void* buffer;
    int length;
    int flags;
};

struct network_info {
    unsigned short dhcp; /* state */
    unsigned int my_ip;
    unsigned int gw;
    unsigned int dns;
};

/* These are the userspace api */
int bind(int socket, const struct sockaddr *address, socklen_t address_len);
int accept(int socket, struct sockaddr *address, socklen_t *address_len);
int connect(int socket, const struct sockaddr *address, socklen_t address_len);
int listen(int socket, int backlog);
int recv(int socket, void *buffer, int length, int flags);
int recvfrom(int socket, void *buffer, int length, int flags, struct sockaddr *address, socklen_t *address_len);
int recv_timeout(int socket, void *buffer, int length, int flags, int timeout);
int send(int socket, const void *message, int length, int flags);
int sendto(int socket, const void *message, int length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
int socket(int domain, int type, int protocol);
void close(int socket);


#ifdef __cplusplus
#include "../../apps/utils/cppUtils.hpp"
#include <util.h>

inline unsigned short htons(unsigned short data)
{
  return (((data & 0x00ff) << 8) |
           (data & 0xff00) >> 8);
}

inline unsigned int htonl(unsigned int data)
{
  return (((data & 0x000000ff) << 24) |
          ((data & 0x0000ff00) << 8)  |
          ((data & 0x00ff0000) >> 8)  |
          ((data & 0xff000000) >> 24) );
}

/* TCP Client */
class TcpClient {

public:
    TcpClient(const char* host, unsigned short port = 80) {
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if (sd != 0) {
            // error
        }

        dest_addr.sin_addr.s_addr = htonl(760180939);
        dest_addr.sin_port = htons(4242);
        dest_addr.sin_family = AF_INET;

        int ret = connect(sd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
        if (ret != 0) {
            // error
        }
    }

    ~TcpClient(){
        close(sd);
    }

    int sendMsg(const char* msg){
        return send(sd, msg, strlen(msg), 0);
    }

    int recvMsg(char* buf, int len){
        return recv(sd, buf, len, 0);
    }

private:
    socket_t sd;
    struct sockaddr_in dest_addr;
};

}
#endif
#endif // !__LIB_NET_H