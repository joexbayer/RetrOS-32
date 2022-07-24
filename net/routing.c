#include <net/routing.h>
#include <net/dhcpd.h>
#include <net/utils.h>

uint32_t route(uint32_t destination)
{
    return htonl(dhcp_get_gw());
}