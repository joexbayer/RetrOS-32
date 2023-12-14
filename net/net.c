#include <net/net.h>
#include <net/tcp.h>
#include <net/socket.h>
#include <serial.h>
#include <assert.h>
#include <scheduler.h>
#include <errors.h>

/**
 * @brief Binds a IP and Port to a socket, mainly used for the server side.
 * 
 * @param socket socket to bind
 * @param address Address to bind
 * @param address_len lenght of bind.
 * @return int 
 */
error_t kernel_bind(struct sock* socket, const struct sockaddr *address, socklen_t address_len)
{
    if(socket->socket > NET_NUMBER_OF_SOCKETS)
        return -ERROR_INVALID_SOCKET;
    
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
error_t kernel_recvfrom(struct sock* socket, void *buffer, int length, int flags, struct sockaddr *address, socklen_t *address_len)
{
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
error_t kernel_recv(struct sock* socket, void *buffer, int length, int flags)
{
    int read = -1;
    switch (socket->type){
    case SOCK_DGRAM:
        break;
    case SOCK_STREAM:
        break;
    default:
        dbgprintf("Unknown socket type\n");
        return -ERROR_INVALID_SOCKET_TYPE;
    }

    dbgprintf(" %d reading from socket ...\n");
    read = net_sock_read(socket, buffer, length);

    dbgprintf("Socket %d recv %d\n", socket->socket, read);

    return read;
}

error_t kernel_recv_timeout(struct sock* socket, void *buffer, int length, int flags, int timeout)
{
    int time_start = get_time();

    int read = -1;
    while(read == -1){
        if(get_time() - time_start > timeout+3)return 0;

    }

    return read;

}

error_t kernel_connect(struct sock* socket, const struct sockaddr *address, socklen_t address_len)
{
    /* Cast sockaddr back to sockaddr_in. Cast originally to comply with linux implementation.*/
    struct sockaddr_in* addr = (struct sockaddr_in*) address;
    
    if(socket->bound_port == 0){
        net_sock_bind(socket, 0, INADDR_ANY);
    }

    assert(socket->tcp == NULL);
    tcp_new_connection(socket, addr->sin_port, socket->bound_port);

    struct sockaddr_in* sptr = &socket->recv_addr;
    memcpy(sptr, addr, sizeof(struct sockaddr_in));

    socket->tcp->state = TCP_SYN_SENT;
    tcp_connect(socket);

    dbgprintf(" [%d] Connecting...\n", socket);
    /* block or spin */
    WAIT(!net_sock_is_established(socket));

    dbgprintf(" [%d] succesfully connected!\n", socket);

    return 0;
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
error_t kernel_sendto(struct sock* socket, const void *message, int length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    /* Flags are ignored... for now. */
    if(socket->socket > NET_NUMBER_OF_SOCKETS){
        return -ERROR_INVALID_SOCKET;
    }

    /* Cast sockaddr back to sockaddr_in. Cast originally to comply with linux implementation.*/
    struct sockaddr_in* addr = (struct sockaddr_in*) dest_addr;
    if(socket->bound_port == 0){
        net_sock_bind(socket, 0, INADDR_ANY);
    }

    dbgprintf("Socket %d sending %d\n", socket->socket, length);
    /* Forward packet to specified protocol. */
    switch (socket->type){
    case SOCK_DGRAM:
        if(net_udp_send((char*) message, BROADCAST_IP, addr->sin_addr.s_addr, ntohs(socket->bound_port), ntohs(addr->sin_port), length) <= 0)
            return 0;
        break;

    case SOCK_STREAM:
        /* TODO */
        break;

    default:
        return -ERROR_INVALID_SOCKET_TYPE;
    }

    socket->tx += length;

    return length;
}

error_t kernel_accept(struct sock* socket, struct sockaddr *address, socklen_t *address_len)
{

    /* accept: only is valid in a TCP connection context. */
    if(!tcp_is_listening(socket))
        return -1;
    
    /* Create new TCP socket? */
    

    return 0;
}

error_t kernel_listen(struct sock* socket, int backlog)
{
    return tcp_set_listening(socket, backlog);
}

error_t kernel_send(struct sock* socket, void *message, int length, int flags)
{
    /* for the time being, only send messages under 1500 bytes */
    if(length > 1400){
        return -ERROR_MSS_SIZE;
    }

    /**
     * This should not be done by the process calling this function. 
     * Instead by another process, perhaps the worker thread.
     * Or simply start a async function. The original thread should simply 
     * block / spin and be notified when data has been sent.
     * 
     * Currently we only accept tiny messages so this is not needed yet...
     */
    WAIT(!net_sock_is_established(socket));
    
    /* TODO: if message was bigger than 1400, send 1400 at a time, simple stop and wait. */
    dbgprintf(" [%d] Sending %d bytes\n", socket->socket, length);
    socket->tcp->state = TCP_WAIT_ACK;
    tcp_send_segment(socket, message, length, 1);

    /* Move this into tcp_send_segment */
    //WAIT(net_sock_awaiting_ack(socket));

    /* Split into smaller "messages" of needed. */
    return length;
}