/**
 * @file ipv4.c
 * @author Joe Bayer (joexbayer)
 * @brief Implementation of the IPv4 protocol.
 * @version 0.1
 * @date 2022-06-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/ipv4.h>
#include <net/utils.h>
#include <terminal.h>
#include <util.h>
#include <net/arp.h>
#include <net/ethernet.h>
#include <net/routing.h>

#include <net/dhcp.h>
#include <serial.h>

/**
 * @brief Creates and attaches IP header to SKB
 * 
 * @param skb skb to modify
 * @param ip destination IP
 * @param proto TCP / UDP
 * @param length length of message.
 * @return int 
 */
int net_ipv4_add_header(struct sk_buff* skb, uint32_t ip, uint8_t proto, uint32_t length)
{
    struct ip_header hdr = {
        .version = IPV4,
        .ihl = 0x05,
        .tos = 0,
        .len = length+hdr.ihl*4,
        .frag_offset = 0x4000,
        .ttl = 64,
        .proto = proto,
        .saddr = dhcp_get_ip(),
        .daddr = ip,
        .csum = 0
    };
    IP_HTONL(&hdr);
    hdr.csum = checksum(&hdr, hdr.ihl * 4, 0);

    skb->proto = IP;

    /* Add ethernet header */
    uint32_t next_hop = route(ip);
	int ret = net_ethernet_add_header(skb, next_hop);
	if(ret < 0){
		dbgprintf("Error adding ethernet header\n");
		return ret;
	}

    /* Add IP header to packet */
    memcpy(skb->data, &hdr, sizeof(struct ip_header));
    skb->len += hdr.ihl * 4;
    skb->data += hdr.ihl * 4;

    dbgprintf("Added IPv4 header.\n");

    return 0;
}

/**
 * @brief parses a packets IP header.
 * 
 * @param skb skb packet to parse
 * @return int 
 */
int net_ipv4_parse(struct sk_buff* skb)
{
    struct ip_header* hdr = (struct ip_header* ) skb->data;
    int hdr_len = hdr->ihl*4;
    skb->hdr.ip = hdr;

    /**
     * @brief Calculate checksum of IP packet
     * and validate that it is correct.
     */
    uint16_t csum = checksum(hdr, hdr_len, 0);
    if(0 != csum){
        dbgprintf("Checksum failed (IPv4)\n");
        return -1;
    }

    IP_NTOHL(hdr);
    skb->data = skb->data+hdr_len;

    if(BROADCAST_IP != ntohl(skb->hdr.ip->daddr) && ntohl(skb->hdr.ip->daddr) != (uint32_t)dhcp_get_ip()){
        dbgprintf("IP mismatch: %d %d\n", skb->hdr.ip->daddr, dhcp_get_ip());
        return -1; /* Currently only accept broadcast packets. */
    }

    char mac[6];
    int arp = net_arp_find_entry(hdr->saddr, (uint8_t*)&mac);
    if(arp < 0){
        struct arp_content content = {
            .sip = hdr->saddr
        };

        struct ethernet_header* ehdr= (struct ethernet_header*) skb->head;
        memcpy(&content.smac, ehdr->smac, 6);

        net_arp_add_entry(&content);
    }

    dbgprintf("[IPv%d] from %i, len: %d, id: %d\n", hdr->version, hdr->saddr, hdr->len, hdr->id);

    return 0;
}