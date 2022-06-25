#include <net/dhcpd.h>
#include <process.h>
#include <net/socket.h>
#include <terminal.h>

socket_t dhcp_socket;

void dhpcd()
{
    dhcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    while(1){};
}

PROGRAM(dhcpd, &dhpcd)
PROGRAM_END