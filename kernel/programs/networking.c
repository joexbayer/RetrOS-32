/**
 * @file networking.c
 * @author Joe Bayer (joexbayer)
 * @brief Main process for handling all networking traffic. 
 * @version 0.1
 * @date 2022-06-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <process.h>
#include <screen.h>

#include <net/netdev.h>
#include <net/packet.h>
#include <net/ethernet.h>

#define MAX_OPEN_PORTS 45
#define MAX_QUEUE_SIZE 20

#define MAX_PACKET_SIZE 0x1000

static struct packet queue[MAX_QUEUE_SIZE];

static uint16_t packets = 0;
static uint32_t ip;
static uint16_t ports[MAX_OPEN_PORTS];
static uint16_t open_ports = 0;

int add_queue(uint8_t* buffer, uint16_t size);
int get_next_queue();

void networking_print_status()
{
    scrwrite(51, 1, "Networking:", VGA_COLOR_CYAN);
    scrprintf(51, 2, "Open Ports: %d", open_ports);
    scrprintf(51, 3, "IPv4: 127.0.0.1");
    scrprintf(51, 4, "MAC: 55:f4:7f:33:h6:l1");
    scrprintf(51, 5, "Packets: %d", packets);
}

void list_net_devices()
{

}

void net_new_packet()
{
    char buffer[MAX_PACKET_SIZE];
    int read = netdev_recieve(&buffer, MAX_PACKET_SIZE);
    if(read <= 0)
        return;
    packets++;
    
    struct ethernet_header* header = (struct ethernet_header*) buffer;
    header->ethertype = ntohs(header->ethertype);

    print_ethernet(header);
}

/**
 * @brief Main networking event loop.
 * 
 */
void main()
{

}

PROGRAM(networking, &main)
ATTACH("lsnet", &list_net_devices)
PROGRAM_END