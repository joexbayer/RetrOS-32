/**
 * @file dhcpd.c
 * @author Joe Bayer (joexbayer)
 * @brief Simple implementation of the DHCP protocol to get a IP, Gateway and DNS server.
 * @version 0.1
 * @date 2022-07-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/dhcp.h>
#include <net/net.h>
#include <net/socket.h>
#include <net/ipv4.h>
#include <net/dns.h>
#include <kutils.h>

#include <scheduler.h>
#include <memory.h>
#include <libc.h>
#include <serial.h>


static struct dhcp_state dhcp_state;
char* dhcp_state_names[4] = {"FAILED", "SUCCESS", "PENDING", "STOPPED"};

/**
 * @brief Adds a option the options part of the DHCP struct.
 * 
 * @param dhcp Struct to add option too.
 * @param offset Option offset
 * @param opcode option code
 * @param opsz option size
 * @param val option value
 * @return int 
 */
static int __dhcp_add_option(struct dhcp* dhcp, int offset, uint8_t opcode, uint8_t opsz, uint8_t* val)
{
    if(offset >= 128)
        return 0;

    uint8_t* ptr = (uint8_t*) &(dhcp->dhcp_options[offset]);
    
    if(255 == opcode){
        *ptr = 255;
        return 1;
    }

    *ptr = opcode;
    ptr++;

    *ptr = opsz;
    ptr++;  

    for (int i = 0; i < opsz; i ++)
        *(ptr+i) = *(val+i);

    return opsz + 2;
}

static int __dhcp_get_option(struct dhcp* dhcp, uint8_t opcode)
{
    int offset = 0;
    uint8_t* ptr = (uint8_t*) &(dhcp->dhcp_options[0]);

    while(*ptr != 255){
        
        if(offset >= 128)
            return 0;

        uint8_t opc = *ptr++;
        uint8_t opsz = *ptr++;

        if(opc == opcode){
            switch (opsz){
            case 1:
                return *ptr;            
            case 2:
                return *((uint16_t*) ptr);
            case 4:
                return *((uint32_t*) ptr);;
            default:
                return -1;
            }
        }
        
        ptr += opsz;
        offset += opsz + 2;
    }

    return -1;
}

/**
 * @brief Creates a DHCP discovery packet sent on broadcast.
 * 
 * @param socket Socket to send with.
 * @return int 
 */
static int __dhcp_send_discovery(struct sock* socket)
{
    int optoff = 0;
    int opt1 = 1;
    int opt0 = 0;
    struct sockaddr_in addr;
    struct dhcp dhcp_disc;

    DHCP_DISCOVERY((&dhcp_disc));
    memcpy(dhcp_disc.dhcp_chaddr, current_netdev.mac, 6);

    optoff += __dhcp_add_option(&dhcp_disc, optoff,  53, 1, (uint8_t *) &opt1);
    optoff += __dhcp_add_option(&dhcp_disc, optoff, 255, 0, (uint8_t *) &opt0);

    addr.sin_port = htons(DHCP_DEST_PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = BROADCAST_IP;

    return kernel_sendto(socket, &dhcp_disc, sizeof(dhcp_disc), 0, (struct sockaddr*) &addr, 0);
}

/**
 * @brief creates a DHCP request packet with proper options.
 * 
 * @param socket Socker to send with.
 * @return int 
 */
static int __dhcp_send_request(struct sock* socket)
{
    int optoff = 0;
    int opt3 = 3;
    int opt0 = 0;
    struct sockaddr_in addr;
    struct dhcp dhcp_req;
    DHCP_REQUEST((&dhcp_req), dhcp_state.gateway, dhcp_state.ip);

    optoff  += __dhcp_add_option(&dhcp_req, optoff, 53,  1, (uint8_t *) &opt3);
    optoff  += __dhcp_add_option(&dhcp_req, optoff, 50,  4, (uint8_t *) &dhcp_state.ip);
    optoff  += __dhcp_add_option(&dhcp_req, optoff, 54,  4, (uint8_t *) &dhcp_state.gateway);
    optoff  += __dhcp_add_option(&dhcp_req, optoff, 255, 0, (uint8_t *) &opt0);

    addr.sin_port = htons(DHCP_DEST_PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = BROADCAST_IP;

    return kernel_sendto(socket, &dhcp_req, sizeof(dhcp_req), 0, (struct sockaddr*) &addr, 0);
}

/**
 * @brief DHCP function for handling a offer given by server.
 * Mostly parsing struct memebers.
 * @param offer dhcp offer from server.
 */
static void __dhcp_handle_offer(struct dhcp* offer)
{
    uint32_t my_ip = offer->dhcp_yiaddr;
    uint32_t server_ip = offer->dhcp_siaddr;

    int dns = __dhcp_get_option(offer, 6);

    dhcp_state.dns = dns;
    dhcp_state.ip = my_ip;
    dhcp_state.gateway = server_ip;
    dhcp_state.state = DHCP_SUCCESS;

    net_configure_iface("eth0", my_ip, 0xFF0000, server_ip);

    dbgprintf("[DHCP] Recieved IP.\n");
}

int dhcp_get_state()
{
    return dhcp_state.state;
}

int dhcp_get_dns()
{
    return dhcp_state.dns;
}


int dhcp_get_ip()
{
    if(dhcp_state.state != DHCP_SUCCESS)
        return -1;

    return dhcp_state.ip;
}

int dhcp_get_gw()
{
    if(dhcp_state.state != DHCP_SUCCESS)
        return -1;

    return dhcp_state.gateway;
}

void __kthread_entry dhcpd()
{
    if(dhcp_state.state == DHCP_SUCCESS)
        return;

    dbgprintf("DHCPD\n");

    int ret;
    /* Create and bind DHCP socket to DHCP_SOURCE_PORT and INADDR_ANY. */
    struct sock* dhcp_socket = kernel_socket_create(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;

    dhcp_state.state = DHCP_PENDING;

    dest_addr.sin_addr.s_addr = INADDR_ANY;
    dest_addr.sin_port = htons(DHCP_SOURCE_PORT);
    dest_addr.sin_family = AF_INET;

    if(kernel_bind(dhcp_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr)) < 0)
        goto dhcp_error;

    /* Send first discovery packet */
    dbgprintf("Sending discovery...\n");
    ret = __dhcp_send_discovery(dhcp_socket);
    if(ret < 0)
        goto dhcp_error;

    char buffer[2048];
    int read = kernel_recv(dhcp_socket, &buffer, 2048, 0);
    while(read < 0){
        __dhcp_send_discovery(dhcp_socket);
        read = kernel_recv(dhcp_socket, &buffer, 2048, 0);
    }

     dbgprintf("DHCPD\n");
    struct dhcp* offer = (struct dhcp*) &buffer;
    __dhcp_handle_offer(offer);

    dbgprintf("Received offer...\n");

    /* Send request after offer was recieved.*/
    ret = __dhcp_send_request(dhcp_socket);
    if(ret < 0)
        goto dhcp_error;

    dbgprintf("Sending request...\n");
    read = kernel_recv(dhcp_socket, &buffer, 2048, 0);
    while(read <= 0){
        __dhcp_send_request(dhcp_socket);
        read = kernel_recv(dhcp_socket, &buffer, 2048, 0);
    }

    dbgprintf("[DHCP] IP: %i\n[DHCP] GW: %i\n[DHCP] DNS: %i\n[DHCP] State: %s\n", dhcp_state.ip, dhcp_state.gateway, dhcp_state.dns);
    
    kernel_sock_close(dhcp_socket);
    
    kernel_exit();

dhcp_error:
    dbgprintf("DCHP ERROR\n");
    dhcp_state.state = DHCP_FAILED;
    kernel_sock_close(dhcp_socket);
    kernel_exit();
    while(1);
}