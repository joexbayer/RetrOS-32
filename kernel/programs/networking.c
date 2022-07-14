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
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/socket.h>
#include <net/dhcpd.h>

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

    scrwrite(51, 3, "DHCP", VGA_COLOR_LIGHT_BLUE);
    int state = dhcp_get_state();
    if(state != DHCP_SUCCESS){
        scrprintf(55, 3, " (%s)      ", dhcp_state_names[state]);
        scrprintf(51, 4, "    IP: %s", "NONE");
        scrprintf(51, 5, "    DNS: %s", "NONE");
        scrprintf(51, 6, "    GW: %s", "NONE");
    } else {
        scrprintf(55, 3, "              ");

        int ip = dhcp_get_ip();
        unsigned char bytes[4];
        bytes[0] = (ip >> 24) & 0xFF;
        bytes[1] = (ip >> 16) & 0xFF;
        bytes[2] = (ip >> 8) & 0xFF;
        bytes[3] = ip & 0xFF;
        scrprintf(51, 4, "    IP: %d.%d.%d.%d     \n", bytes[3], bytes[2], bytes[1], bytes[0]);

        int dns = dhcp_get_dns();
        unsigned char bytes_dns[4];
        bytes_dns[0] = (dns >> 24) & 0xFF;
        bytes_dns[1] = (dns >> 16) & 0xFF;
        bytes_dns[2] = (dns >> 8) & 0xFF;
        bytes_dns[3] = dns & 0xFF;
        scrprintf(51, 5, "    DNS: %d.%d.%d.%d     \n", bytes_dns[3], bytes_dns[2], bytes_dns[1], bytes_dns[0]);
        
        int gw = dhcp_get_gw();
        unsigned char bytes_gw[4];
        bytes_gw[0] = (gw >> 24) & 0xFF;
        bytes_gw[1] = (gw >> 16) & 0xFF;
        bytes_gw[2] = (gw >> 8) & 0xFF;
        bytes_gw[3] = gw & 0xFF;
        scrprintf(51, 6, "    GW: %d.%d.%d.%d     \n", bytes_gw[3], bytes_gw[2], bytes_gw[1], bytes_gw[0]);
    }

    scrprintf(51, 8, "MAC: %x:%x:%x:%x:%x:%x", current_netdev.mac[0], current_netdev.mac[1], current_netdev.mac[2], current_netdev.mac[3], current_netdev.mac[4], current_netdev.mac[5]);
    scrprintf(51, 9, "Packets: %d", packets);
    scrprintf(51, 10, "Sockets: %d", get_total_sockets());

}

void list_net_devices()
{

}

void net_handle_send(struct sk_buff* skb)
{
    int read = netdev_transmit(skb->head, skb->len);
    if(read <= 0){
        twriteln("Error sending packet.");
    }
    packets++;
}

int net_drop_packet(struct sk_buff* skb)
{
    current_netdev.dropped++;
    FREE_SKB(skb);

    return 0;
}

void net_packet_handler()
{
    struct sk_buff* skb = get_skb();
    ALLOCATE_SKB(skb);
    skb->action = RECIEVE;

    int read = netdev_recieve(skb->data, MAX_PACKET_SIZE);
    if(read <= 0) {
        FREE_SKB(skb);
        return;
    }
    skb->len = read;
    packets++;
}

int net_handle_recieve(struct sk_buff* skb)
{
    int ret = ethernet_parse(skb);
    if(ret <= 0)
        return net_drop_packet(skb);

    switch(skb->hdr.eth->ethertype){
        case IP:
            if(!ip_parse(skb))
                return net_drop_packet(skb);
            
            twriteln("Recieved IP packet.");
            switch (skb->hdr.ip->proto)
            {
            case UDP:
                ret = udp_parse(skb);
                break;
            
            case ICMPV4:
                ret = icmp_parse(skb);
                break;
            default:
                return net_drop_packet(skb);
            }
            break;

        case ARP:
            if(!arp_parse(skb))
                return net_drop_packet(skb);

            // send arp response.
            twriteln("Recieved ARP packet.");
            break;

        default:
            return net_drop_packet(skb);
    }

    FREE_SKB(skb);

    return 1;
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

        switch (skb->action)
        {
        case RECIEVE:
            net_handle_recieve(skb);
            break;
        case SEND:
            net_handle_send(skb);
            twriteln("Sending Packet!");
            break;
        default:
            break;
        }

        FREE_SKB(skb);
    }
}

PROGRAM(networking, &main)
ATTACH("lsnet", &list_net_devices)
ATTACH("arp -a", &arp_print_cache);
ATTACH("arp", &arp_request)
PROGRAM_END