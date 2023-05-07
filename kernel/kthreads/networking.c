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
#include <assert.h>
#include <kthreads.h>
#include <work.h>

#include <net/netdev.h>
#include <net/net.h>
#include <net/packet.h>
#include <net/skb.h>
#include <net/ethernet.h>
#include <net/ipv4.h>
#include <net/tcp.h>
#include <net/icmp.h>
#include <net/socket.h>
#include <net/net.h>
#include <net/dhcp.h>

#define MAX_PACKET_SIZE 0x600

enum NETD_STATES {
    NETD_UNINITIALIZED,
    NETD_STARTED
};

static struct network_manager {
    int state;

    uint16_t packets;
    struct skb_queue* skb_tx_queue;
    struct skb_queue* skb_rx_queue;

    struct net_info stats;

} netd = {
    .state = NETD_UNINITIALIZED,
    .packets = 0,
    .stats.dropped = 0, 
    .stats.recvd = 0,
    .stats.sent = 0
};

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
    netd.packets++;
    netd.stats.recvd++;
    dbgprintf("New packet incoming...\n");
}

void net_send_skb(struct sk_buff* skb)
{
    /* Validate SKB */
    netd.skb_tx_queue->ops->add(netd.skb_tx_queue, skb);
    netd.packets++;
    dbgprintf("Added SKB to TX queue\n");
}

static void __net_transmit_skb(struct sk_buff* skb)
{
    dbgprintf("Transmitting packet\n");
    int read = netdev_transmit(skb->head, skb->len);
    if(read <= 0){
        dbgprintf("Error sending packet\n");
    }
    netd.packets++;
    netd.stats.sent++;
}

int net_drop_packet(struct sk_buff* skb)
{
    current_netdev.dropped++;
    netd.stats.dropped++;
    skb_free(skb);
    return 0;
}

error_t net_get_info(struct net_info* info)
{
    *info = netd.stats;
    return ERROR_OK;
}

int net_handle_recieve(struct sk_buff* skb)
{
    dbgprintf("Parsing new packet\n");
    if(net_ethernet_parse(skb) < 0) return net_drop_packet(skb);
    switch(skb->hdr.eth->ethertype){
        /* Ethernet type is IP */
        case IP:
            if(net_ipv4_parse(skb) < 0) return net_drop_packet(skb);
            switch (skb->hdr.ip->proto){
            case UDP:
                if(net_udp_parse(skb) < 0) return net_drop_packet(skb);
                break;
            
            case TCP:
                if(tcp_parse(skb) < 0) return net_drop_packet(skb);
                skb_free(skb);
                break;
            
            case ICMPV4:
                if(net_icmp_parse(skb) < 0) return net_drop_packet(skb);
                net_icmp_handle(skb);
                skb_free(skb);
                break;
            default:
                return net_drop_packet(skb);
            }
            break;

        /* Ethernet type is ARP */
        case ARP:
            if(arp_parse(skb) < 0) return net_drop_packet(skb);
            // send arp response.
            dbgprintf("Recieved ARP packet.\n");
            skb_free(skb);
            break;

        default:
            return net_drop_packet(skb);
    }
    return 1;
}

/**
 * @brief Main networking event loop.
 * 
 */
void networking_main()
{
    if(!is_netdev_attached()) return;

    if(netd.state == NETD_UNINITIALIZED){
        netd.skb_rx_queue = skb_new_queue();
        netd.skb_tx_queue = skb_new_queue();
    }

    start("dhcpd");

    while(1){
        /**
         * @brief Query RX and TX queue for netd.packets.
         */
        if(SKB_QUEUE_READY(netd.skb_tx_queue)){
            dbgprintf("Sending new SKB from TX queue\n");
            struct sk_buff* skb = netd.skb_tx_queue->ops->remove(netd.skb_tx_queue);
            assert(skb != NULL);
            __net_transmit_skb(skb);
            skb_free(skb);
        }

        if(SKB_QUEUE_READY(netd.skb_rx_queue)){
            dbgprintf("Receiving new SKB from RX queue\n");
            struct sk_buff* skb = netd.skb_rx_queue->ops->remove(netd.skb_rx_queue);
            assert(skb != NULL);

            /* Offload skb parsing to worker thread. */
            work_queue_add(&net_handle_recieve, (void*)skb, NULL);
        }
    }
}