#include <net/dhcpd.h>
#include <process.h>
#include <net/socket.h>
#include <net/ipv4.h>
#include <terminal.h>
#include <memory.h>
#include <util.h>

static socket_t dhcp_socket;
static struct dhcp_state dhcp_state;


static int dhcp_add_option(struct dhcp* dhcp, int offset, uint8_t opcode, uint8_t opsz, uint8_t* val)
{
    if(offset >= 128)
        return 0;

    uint8_t* ptr = (uint8_t*) (&dhcp->dhcp_options)+offset;
    ptr += sizeof(struct dhcp);
    
    if(255 == opcode){
        *ptr = 255;
        return 1;
    }

    *ptr = opcode;
    ptr++;

    *ptr = opsz;
    ptr++;  

    for (int i = 0; i < opsz; i ++)
        ptr[i] = val[i];

    return opsz + 2;
}

void dhcpd()
{
    dhcp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    dhcp_state.state = DHCP_PENDING;

    int optoff = 0;
    int opt1 = 1;
    int opt0 = 0;
    struct dhcp* dhcp_disc = alloc(sizeof(struct dhcp));
    DHCP_DISCOVERY(dhcp_disc);

    optoff += dhcp_add_option(dhcp_disc, optoff,  53, 1, (uint8_t *) &opt1);
    optoff += dhcp_add_option(dhcp_disc, optoff, 255, 0, (uint8_t *) &opt0);

    twritef("%d %d\n", optoff, sizeof(*dhcp_disc));

    struct sockaddr_in addr;
    addr.sin_port = htons(DHCP_DEST_PORT);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = BROADCAST_IP;
    int ret = sendto(dhcp_socket, dhcp_disc, sizeof(*dhcp_disc)+optoff, 0, (struct sockaddr*) &addr, 0);



    while(1){};
}

PROGRAM(dhcpd, &dhcpd)
dhcp_state.ip = 0;
dhcp_state.state = DHCP_STOPPED;
dhcp_state.tries = 0;
PROGRAM_END