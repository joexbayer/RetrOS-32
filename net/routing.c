#include <net/routing.h>
#include <net/dhcp.h>
#include <net/utils.h>
#include <net/net.h>

uint32_t route(uint32_t destination)
{
    if(destination == htonl(LOOPBACK_IP))
        return LOOPBACK_IP;
    return htonl(dhcp_get_gw());
}