/**
 * @file socket.c
 * @author Joe Bayer (joexbayer)
 * @brief Socket functions for networking.
 * @version 0.1
 * @date 2022-06-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/socket.h>
#include <memory.h>

#define MAX_NUMBER_OF_SOCKETS 128

static struct sock** sockets;
static int total_sockets;

/**
 * Example:
 * 
 * struct sockaddr_in myaddr;
 * int s;
 *
 * myaddr.sin_family = AF_INET;
 * myaddr.sin_port = htons(3490);
 * inet_aton("63.161.169.137", &myaddr.sin_addr.s_addr);
 * 
 * s = socket(PF_INET, SOCK_STREAM, 0);
 * bind(s, (struct sockaddr*)myaddr, sizeof(myaddr));
 * 
 */

int get_total_sockets()
{
    return total_sockets;
}

/**
 * @brief Reads data from socket and creates a sockaddr of sender. 
 * Mainly used for UDP.
 * @param socket Socket to read from
 * @param buffer Buffer to store message.
 * @param length Length of buffer.
 * @param flags Flags..
 * @param address sockaddr of sender.
 * @param address_len lenght of address.
 * @return size_t 
 */ 
size_t recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{

}

/**
 * @brief Sends data to reciever defined by sockaddr.
 * 
 * @param socket Socket used to send.
 * @param message buffer to send.
 * @param length length of message
 * @param flags flags..
 * @param dest_addr sockaddr defining reciever.
 * @param dest_len lenght of dest_addr.
 * @return size_t 
 */
size_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{

}

int udp_deliver_packet(uint32_t ip, uint16_t port, char* buffer, uint16_t len)
{
    for (int i = 0; i < total_sockets; i++)
    {
        if(sockets[i]->recv_addr.sin_port == htons(port) && sockets[i]->recv_addr.sin_addr.s_addr == ip)
        {
            
        }
    }


    
}


/**
 * @brief Creates a socket and allocates a struct sock representation.
 * Needed for the network stack to forward data to correct socket.
 * @param domain The domain of the socket
 * @param type Type of the socket
 * @param protocol Protocol (UDP / TCP)
 * @return socket_t 
 */
socket_t socket(int domain, int type, int protocol)
{
    int current = total_sockets;

    sockets[current] = alloc(sizeof(struct sock)); /* Allocate space for a socket. Needs to be freed. */
    sockets[current]->domain = domain;
    sockets[current]->protocol = protocol;
    sockets[current]->type = type;
    sockets[current]->socket = current;
    sockets[current]->buffer_len = 0;

    mutex_init(&(sockets[current]->sock_lock));

    total_sockets++;
    return (socket_t) current;
}

void init_sockets()
{
    sockets = (struct sock**) alloc(MAX_NUMBER_OF_SOCKETS * sizeof(void*));
    total_sockets = 0;
}