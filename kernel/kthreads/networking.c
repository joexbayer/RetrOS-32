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

#include <scheduler.h>
#include <serial.h>

#include <net/netdev.h>
#include <net/net.h>
#include <net/packet.h>
#include <net/skb.h>
#include <net/ethernet.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/socket.h>
#include <net/dhcp.h>

#define MAX_PACKET_SIZE 0x600
static uint16_t packets = 0;

struct network_manager {
    int state;

    struct skb_queue* skb_tx_queue;
    struct skb_queue* skb_rx_queue;
} netd;

void net_incoming_packet_handler()
{
    struct sk_buff* skb = skb_new();
    skb->len = netdev_recieve(skb->data, MAX_PACKET_SIZE);
    if(skb->len <= 0) {
        dbgprintf("Received an empty packet.\n");
        skb_free(skb);
        return;
    }

    netd.skb_rx_queue->ops->add(netd.skb_rx_queue, skb);
    packets++;
}

void net_send_skb(struct sk_buff* skb)
{
    /* Validate SKB */
    netd.skb_tx_queue->ops->add(netd.skb_tx_queue, skb);
    packets++;
}

static void __net_transmit_skb(struct sk_buff* skb)
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
    
    skb_free(skb);

    return 0;
}

int net_handle_recieve(struct sk_buff* skb)
{
    int ret;

    if(!ethernet_parse(skb))
        return net_drop_packet(skb);

    switch(skb->hdr.eth->ethertype){
        /* Ethernet type is IP */
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

        /* Ethernet type is ARP */
        case ARP:
            if(!arp_parse(skb))
                return net_drop_packet(skb);

            // send arp response.
            dbgprintf("Recieved ARP packet.\n");
            break;

        default:
            return net_drop_packet(skb);
    }

    skb_free(skb);

    return 1;
}

/**
 * @brief Main networking event loop.
 * 
 */
void networking_main()
{
    /* Maybe move these out into a init function */
    netd.skb_rx_queue = skb_new_queue();
    netd.skb_tx_queue = skb_new_queue();

    while(1)
    {
        /**
         * @brief Query RX and TX queue for packets.
         */

        if(SKB_QUEUE_READY(netd.skb_tx_queue))
        {
            struct sk_buff* skb = netd.skb_tx_queue->ops->remove(netd.skb_tx_queue);
            __net_transmit_skb(skb);
            skb_free(skb);
        }

        if(SKB_QUEUE_READY(netd.skb_rx_queue))
        {
            /* Let a work handle the parsing */
        }
    }
}