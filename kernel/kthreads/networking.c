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

#include <terminal.h>
#include <scheduler.h>
#include <serial.h>

#include <net/netdev.h>
#include <net/packet.h>
#include <net/skb.h>
#include <net/ethernet.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/socket.h>
#include <net/dhcp.h>
#include <windowmanager.h>


#define MAX_PACKET_SIZE 0x900

static uint16_t packets = 0;

int add_queue(uint8_t* buffer, uint16_t size);
int get_next_queue();

void networking_print_status()
{
    /*twriteln("DHCP");
    int state = dhcp_get_state();
    if(state != DHCP_SUCCESS){
        twritef(" (%s)      \n", dhcp_state_names[state]);
        twritef(" IP:  %s\n", "N/A");
        twritef(" DNS: %s\n", "N/A");
        twritef(" GW:  %s\n", "N/A");
    } else {
        int ip = dhcp_get_ip();
        twritef(" IP:  %i     \n", ip);

        int dns = dhcp_get_dns();
        twritef(" DNS: %i     \n", dns);
        
        int gw = dhcp_get_gw();
        twritef(" GW:  %i     \n", gw);
    }

    twritef(" MAC: %x:%x:%x:%x:%x:%x\n", current_netdev.mac[0], current_netdev.mac[1], current_netdev.mac[2], current_netdev.mac[3], current_netdev.mac[4], current_netdev.mac[5]);
    twritef(" Packets: %d\n", packets);
    twritef(" Sockets: %d\n", get_total_sockets());*/

}

void list_net_devices()
{

}

void net_handle_send(struct sk_buff* skb)
{
    int read = netdev_transmit(skb->head, skb->len);
    if(read <= 0){
        dbgprintf("Error sending packet\n");
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
            dbgprintf("Recieved ARP packet.\n");
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
void networking_main()
{
    //attach_window(NULL);
    while(1)
    {
        struct sk_buff* skb = next_skb();
        if(skb == NULL){
            yield();
            continue;
        }
        skb->stage = IN_PROGRESS;

        switch (skb->action)
        {
        case RECIEVE:
            net_handle_recieve(skb);
            break;
        case SEND:
            net_handle_send(skb);
            break;
        default:
            break;
        }

        FREE_SKB(skb);
    }
}