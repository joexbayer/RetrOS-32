/**
 * @file ethernet.c
 * @author Joe Bayer (joexbayer)
 * @brief Handles ethernet parsing, routing.
 * @version 0.1
 * @date 2022-06-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <net/ethernet.h>
#include <kconfig.h>
#include <serial.h>

#ifndef KDEBUG_ETH
#undef dbgprintf
#define dbgprintf(...)
#endif // !KDEBUG_ETH

static void net_ethernet_print(struct ethernet_header* hdr)
{
    dbgprintf("Ethernet Destination: %x %x %x %x %x %x\n", hdr->dmac[0], hdr->dmac[1], hdr->dmac[2], hdr->dmac[3], hdr->dmac[4], hdr->dmac[5]);
    dbgprintf("Ethernet Source: %x %x %x %x %x %x\n", hdr->smac[0], hdr->smac[1], hdr->smac[2], hdr->smac[3], hdr->smac[4], hdr->smac[5]);
}

int net_ethernet_add_header(struct sk_buff* skb, uint32_t ip)
{
    skb->len += ETHER_HDR_LENGTH;
    struct ethernet_header e_hdr = {
        .ethertype = htons(skb->proto)
    };

    int ret = net_arp_find_entry(ntohl(ip), (uint8_t*)&e_hdr.dmac);
    if(ret < 0) return ret;

    memcpy(&e_hdr.smac, skb->interface->device->mac, 6);
    memcpy(skb->data, &e_hdr, ETHER_HDR_LENGTH);
    skb->data += ETHER_HDR_LENGTH;

    //net_ethernet_print(&e_hdr);

    dbgprintf("Added Ethernet header\n");

    return 0;
}

int8_t net_ethernet_parse(struct sk_buff* skb)
{
    struct ethernet_header* header = (struct ethernet_header*) skb->data;
    header->ethertype = ntohs(header->ethertype);

    skb->hdr.eth = header;
    skb->data += ETHER_HDR_LENGTH;

    uint8_t broadcastmac[] = {255, 255, 255, 255, 255, 255};
    if(memcmp(header->dmac, skb->interface->device->mac, 6) == 0 || memcmp(skb->hdr.eth->dmac, (uint8_t*)&broadcastmac, 6) == 0){
        dbgprintf("Ethernet packet for us.\n");
		return 0;
	}

    net_ethernet_print(header);

	return -1;
}