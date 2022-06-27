#include <net/dhcpd.h>
#include <process.h>
#include <net/socket.h>
#include <net/ipv4.h>
#include <terminal.h>
#include <util.h>

socket_t dhcp_socket;

void dhcpd()
{
    dhcp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_port = htons(20);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = BROADCAST_IP;

    char* message = "Hello world!";

    int ret = sendto(dhcp_socket, message, strlen(message), 0, (struct sockaddr*) &addr, 0);

    twritef("%d\n", ret);

    while(1){};
}

PROGRAM(dhcpd, &dhcpd)
PROGRAM_END