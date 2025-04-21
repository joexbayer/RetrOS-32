/**
 * @file routing.c
 * @author Joe Bayer (joexbayer)
 * @brief Routing for internal networking.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <net/routing.h>
#include <net/dhcp.h>
#include <net/utils.h>
#include <net/net.h>

uint32_t route(uint32_t destination)
{
    if(destination == htonl(LOOPBACK_IP)){
        return LOOPBACK_IP;
    }
    
    //dbgprintf("Routing %i to %i\n", ntohl(destination), dhcp_get_gw());
    return htonl(dhcp_get_gw());
}