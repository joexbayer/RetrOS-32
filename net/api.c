#include <net/net.h>
#include <net/socket.h>
#include <serial.h>

/**
 * @brief Binds a IP and Port to a socket, mainly used for the server side.
 * 
 * @param socket socket to bind
 * @param address Address to bind
 * @param address_len lenght of bind.
 * @return int 
 */
int kernel_bind(struct sock* socket, const struct sockaddr *address, socklen_t address_len)
{
    if(socket->socket > MAX_NUMBER_OF_SOCKETS)
        return -1;
    
    /*Cast sockaddr back to sockaddr_in. Cast originally to comply with linux implementation.*/
    struct sockaddr_in* addr = (struct sockaddr_in*) address;

    net_sock_bind(socket, addr->sin_port, addr->sin_addr.s_addr);
    return 1;
}

/**
 * @brief Reads data from socket and creates a sockaddr of sender. 
 * Mainly used for UDP.
 * @param socket Socket to read from
 * @param buffer Buffer to store message.
 * @param length length of buffer.
 * @param flags Flags..
 * @param address sockaddr of sender.
 * @param address_len length of address.
 * @return int 
 */ 
int kernel_recvfrom(struct sock* socket, void *buffer, int length, int flags, struct sockaddr *address, socklen_t *address_len)
{
    /* */
    return 0;
}

/**
 * @brief Simple recieve on socket without sockaddr.
 * 
 * @param socket Socket to read from
 * @param buffer Buffer to copy data into
 * @param length Max length to copy.
 * @param flags flags..
 * @return int 
 */
int kernel_recv(struct sock* socket, void *buffer, int length, int flags)
{
    int read = -1;
    while(read == -1) /* TODO: Block, instead of spinning. */
    {
        read = net_sock_read_skb(socket, buffer);
    }

    dbgprintf("Socket %d recv %d\n", socket->socket, read);

    return read;
}

int kernel_recv_timeout(struct sock* socket, void *buffer, int length, int flags, int timeout)
{
    int time_start = get_time();

    int read = -1;
    while(read == -1) /* TODO: Block, instead of spinning. */
    {
        if(get_time() - time_start > timeout+3)
            return 0;

        read = net_sock_read_skb(socket, buffer);
    }

    return read;

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
 * @return int 
 */
int kernel_sendto(struct sock* socket, const void *message, int length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    /* Flags are ignored... for now. */
    if(socket->socket > MAX_NUMBER_OF_SOCKETS)
        return -1;

    /* Cast sockaddr back to sockaddr_in. Cast originally to comply with linux implementation.*/
    struct sockaddr_in* addr = (struct sockaddr_in*) dest_addr;
    if(socket->bound_port == 0)
        net_sock_bind(socket, 0, INADDR_ANY);

    dbgprintf("Socket %d sending %d\n", socket->socket, length);
    /* Forward packet to specified protocol. */
    switch (socket->type)
    {
    case SOCK_DGRAM:
        if(net_udp_send((char*) message, BROADCAST_IP, addr->sin_addr.s_addr, ntohs(socket->bound_port), ntohs(addr->sin_port), length) <= 0)
            return 0;
        break;

    case SOCK_STREAM:
        /* TODO */
        break;

    default:
        return -1;
    }

    return length;
}