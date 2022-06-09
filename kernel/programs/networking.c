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
#include <terminal.h>

#include <net/netdev.h>
#include <net/packet.h>
#include <net/skb.h>
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
    scrprintf(51, 4, "MAC: %x:%x:%x:%x:%x:%x", current_netdev.mac[0], current_netdev.mac[1], current_netdev.mac[2], current_netdev.mac[3], current_netdev.mac[4], current_netdev.mac[5]);
    scrprintf(51, 5, "Packets: %d", packets);
}

void list_net_devices()
{

}

void net_packet_handler()
{
    struct sk_buff* skb = get_skb();
    ALLOCATE_SKB(skb);

    int read = netdev_recieve(skb->data, MAX_PACKET_SIZE);
    if(read <= 0) {
        FREE_SKB(skb);
        return;
    }
    packets++;
}

/**
 * @brief Main networking event loop.
 * 
 */
void main()
{
    while(1)
    {
        struct sk_buff* skb = next_skb();
        if(skb == NULL)
            continue;        
        skb->stage = IN_PROGRESS;

        int ret = parse_ethernet(skb);
        if(ret <= 0)
            goto drop;

        switch(skb->hdr.eth->ethertype){
			case IP:
				//ip_parse(skb);
                twriteln("Recieved ARP packet.");
				break;

			case ARP:
                ;
				int ret = arp_parse(skb);
                if(!ret)
                    goto drop;

                // send arp response.
                twriteln("Recieved ARP packet.");
				return;

			default:
                goto drop;
		}
    drop:
        FREE_SKB(skb);
    }
}

PROGRAM(networking, &main)
ATTACH("lsnet", &list_net_devices)
PROGRAM_END