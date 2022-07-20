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

#include <net/dhcpd.h>
#include <process.h>
#include <pcb.h>
#include <net/socket.h>
#include <net/ipv4.h>
#include <terminal.h>
#include <memory.h>
#include <util.h>

static struct dhcp_state dhcp_state;
char* dhcp_state_names[4] = {"FAILED", "SUCCESS", "PENDING", "Not Running"};

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
    twritef("Default: %x\n", dhcp->dhcp_cookie);
    int offset = 0;
    uint8_t* ptr = (uint8_t*) &(dhcp->dhcp_options[0]);

    while(*ptr != 255){
        
        if(offset >= 128)
            return 0;

        uint8_t opc = *ptr++;
        uint8_t opsz = *ptr++;

        if(opc == opcode){
            switch (opsz)
            {
            case 1:
                return *ptr;
                break;
            
            case 2:
                return *((uint16_t*) ptr);
                break;
            
            case 4:
                return *((uint32_t*) ptr);;
                break;
            
            default:
                return -1;
                break;
            }
        }
        
        ptr += opsz;
        offset += opsz + 2;
    }

    return 0;
}

/**
 * @brief Creates a DHCP discovery packet sent on broadcast.
 * 
 * @param socket Socket to send with.
 * @return int 
 */
static int __dhcp_send_discovery(socket_t socket)
{
    int optoff = 0;
    int opt1 = 1;
    int opt0 = 0;
    struct dhcp* dhcp_disc = alloc(sizeof(struct dhcp));
    DHCP_DISCOVERY((dhcp_disc));

    optoff += __dhcp_add_option(dhcp_disc, optoff,  53, 1, (uint8_t *) &opt1);
    optoff += __dhcp_add_option(dhcp_disc, optoff, 255, 0, (uint8_t *) &opt0);

    twritef("%d %d\n", optoff, sizeof(*dhcp_disc));

    struct sockaddr_in addr;
    addr.sin_port = htons(DHCP_DEST_PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = BROADCAST_IP;
    int ret = sendto(socket, dhcp_disc, sizeof(*dhcp_disc), 0, (struct sockaddr*) &addr, 0);

    free(dhcp_disc);

    return ret;
}

/**
 * @brief creates a DHCP request packet with proper options.
 * 
 * @param socket Socker to send with.
 * @return int 
 */
static int __dhcp_send_request(socket_t socket)
{
    int optoff = 0;
    int opt3 = 3;
    int opt0 = 0;
    struct dhcp* dhcp_req = alloc(sizeof(struct dhcp));
    DHCP_REQUEST(dhcp_req, dhcp_state.gateway, dhcp_state.ip);

    optoff  += __dhcp_add_option(dhcp_req, optoff, 53,  1, (uint8_t *) &opt3);
    optoff  += __dhcp_add_option(dhcp_req, optoff, 50,  4, (uint8_t *) &dhcp_state.ip);
    optoff  += __dhcp_add_option(dhcp_req, optoff, 54,  4, (uint8_t *) &dhcp_state.gateway);
    optoff  += __dhcp_add_option(dhcp_req, optoff, 255, 0, (uint8_t *) &opt0);

    struct sockaddr_in addr;
    addr.sin_port = htons(DHCP_DEST_PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = BROADCAST_IP;

    int ret = sendto(socket, dhcp_req, sizeof(*dhcp_req), 0, (struct sockaddr*) &addr, 0);

    free(dhcp_req);

    return ret;
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

    twriteln("[DHCP] Recieved IP.");
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

void dhcpd()
{

    int ret;
    /* Create and bind DHCP socket to DHCP_SOURCE_PORT and INADDR_ANY. */
    socket_t dhcp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    dhcp_state.state = DHCP_PENDING;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = INADDR_ANY;
    dest_addr.sin_port = htons(DHCP_SOURCE_PORT);
    dest_addr.sin_family = AF_INET;

    if(bind(dhcp_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr)) < 0)
        goto dhcp_error;


    /* Send first discovery packet */
    ret = __dhcp_send_discovery(dhcp_socket);
    if(ret <= 0)
        goto dhcp_error;

    
    char buffer[2048];
    int read = recv(dhcp_socket, &buffer, 2048, 0);
    if(read <= 0)
        goto dhcp_error;

    struct dhcp* offer = (struct dhcp*) &buffer;
    __dhcp_handle_offer(offer);

    /* Send request after offer was recieved.*/
    ret = __dhcp_send_request(dhcp_socket);
    if(ret <= 0)
        goto dhcp_error;

    read = recv(dhcp_socket, &buffer, 2048, 0);
    if(read <= 0)
        goto dhcp_error; 

    twriteln("DHCP done!");
    
    exit();

dhcp_error:
    dhcp_state.state = DHCP_FAILED;
    exit();
    while(1);
}

PROGRAM(dhcpd, &dhcpd)
dhcp_state.ip = 0;
dhcp_state.state = DHCP_STOPPED;
dhcp_state.tries = 0;
PROGRAM_END