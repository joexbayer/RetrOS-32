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
#include <net/utils.h>
#include <net/skb.h>
#include <memory.h>
#include <bitmap.h>
#include <util.h>

#define MAX_NUMBER_OF_SOCKETS 128
#define DYNAMIC_PORT_START 49152
#define NUMBER_OF_DYMANIC_PORTS 16383

static struct sock** sockets;
static int total_sockets;
static bitmap_t port_map;

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
inline static uint16_t __get_free_port()
{
    return (get_free_bitmap(port_map, NUMBER_OF_DYMANIC_PORTS) + DYNAMIC_PORT_START);
}

inline static void __socket_bind(int socket, uint16_t port, uint32_t ip)
{
    sockets[socket]->bound_ip = ip;
    /* if given port is 0 chose a random one. */
    sockets[socket]->bound_port = port == 0 ? __get_free_port() : port;
}


/**
 * @brief Adds a packets buffer to the sockets buffer, also confirms its lenght.
 * 
 * @param buffer Buffer to copy into sock.
 * @param len Length of buffer.
 * @param socket_index Index of socket to add buffer too.
 * @return int 
 */
static int __sock_add_packet(char* buffer, uint16_t len, int socket_index)
{

    acquire(&sockets[socket_index]->sock_lock);

    if(len > 2048) /* TODO: set as constant. or not? */
        return -1;

    for (size_t i = 0; i < BUFFERS_PER_SOCKET; i++)
    {
        if(sockets[socket_index]->buffer_lens[i] == 0)
        {
            memcpy(sockets[socket_index]->buffers[i], buffer, len);
            sockets[socket_index]->buffer_lens[i] = len;
            release(&sockets[socket_index]->sock_lock);
            return 1;
        }
    }

    release(&sockets[socket_index]->sock_lock);
    return -1;
}

inline static int __sock_read_packet(int index, char* buffer)
{
    acquire(&sockets[index]->sock_lock);

    int next = sockets[index]->last_read_buffer;
    if(sockets[index]->buffer_lens[next] <= 0){
        release(&sockets[index]->sock_lock);
        return -1;
    }
    
    int read = sockets[index]->buffer_lens[next];
    memcpy(buffer, sockets[index]->buffers[next], read);
    sockets[index]->buffer_lens[next] = 0;
    sockets[index]->last_read_buffer = (sockets[index]->last_read_buffer + 1) % BUFFERS_PER_SOCKET;

    release(&sockets[index]->sock_lock);

    return read;
}


int get_total_sockets()
{
    return total_sockets;
}

/**
 * @brief Binds a IP and Port to a socket, mainly used for the server side.
 * 
 * @param socket socket to bind
 * @param address Address to bind
 * @param address_len lenght of bind.
 * @return int 
 */
int bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
    if(socket < 0 || socket > total_sockets)
        return -1;
    
    /*Cast sockaddr back to sockaddr_in. Cast originally to comply with linux implementation.*/
    struct sockaddr_in* addr = (struct sockaddr_in*) address;

    __socket_bind(socket, addr->sin_port, addr->sin_addr.s_addr);

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
 * @return size_t 
 */ 
size_t recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
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
 * @return size_t 
 */
size_t recv(int socket, void *buffer, size_t length, int flags)
{
    int read = -1;
    while(read == -1) /* TODO: Block, instead of spinning. */
    {
        read = __sock_read_packet(socket, buffer);
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
 * @return size_t 
 */
size_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
    /* Flags are ignored... for now. */
    if(socket < 0 || socket > total_sockets)
        return -1;

    /* Cast sockaddr back to sockaddr_in. Cast originally to comply with linux implementation.*/
    struct sockaddr_in* addr = (struct sockaddr_in*) dest_addr;
    if(sockets[socket]->bound_port == 0)
        __socket_bind(socket, __get_free_port(), INADDR_ANY);

    /* Get new SKB for packet. */
	struct sk_buff* skb = get_skb();
    ALLOCATE_SKB(skb);
    skb->stage = IN_PROGRESS;

    /* Forward packet to specified protocol. */
    switch (sockets[socket]->type)
    {
    case SOCK_DGRAM:
        if(udp_send(skb, (char*) message, BROADCAST_IP, addr->sin_addr.s_addr, sockets[socket]->bound_port, ntohs(addr->sin_port), length) <= 0)
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


/**
 * @brief Interface for UDP to add packet to socket if it exists.
 * @todo: Instead of passing IP and Port, pass socketaddr
 * @param ip Destination ip
 * @param port Desination port
 * @param buffer buffer to copy over to socket.
 * @param len lenght of buffer.
 * @return int 
 */
int udp_deliver_packet(uint32_t ip, uint16_t port, char* buffer, uint16_t len) 
{
    for (int i = 0; i < total_sockets; i++){
        twritef("Compare %d -> %d (%d -> %d) \n", sockets[i]->bound_port, htons(port), ntohs(sockets[i]->bound_port), port);
        if(sockets[i]->bound_port == htons(port) && (sockets[i]->bound_ip == ip || sockets[i]->bound_ip == INADDR_ANY))
            return __sock_add_packet(buffer, len, i);
    }

    return -1;
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
    sockets[current]->bound_port = 0;
    sockets[current]->last_read_buffer = 0;

    for (int i = 0; i < BUFFERS_PER_SOCKET; i++)
    {
        sockets[current]->buffer_lens[i] = 0;
    }
    

    mutex_init(&(sockets[current]->sock_lock));

    total_sockets++;
    return (socket_t) current;
}

void init_sockets()
{
    sockets = (struct sock**) alloc(MAX_NUMBER_OF_SOCKETS * sizeof(void*));
    port_map = create_bitmap(NUMBER_OF_DYMANIC_PORTS);
    total_sockets = 0;
}