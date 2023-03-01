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
#include <terminal.h>

int ethernet_add_header(struct sk_buff* skb, uint32_t ip)
{
    skb->len += ETHER_HDR_LENGTH;
    struct ethernet_header e_hdr;

    e_hdr.ethertype = skb->proto;
    e_hdr.ethertype = htons(e_hdr.ethertype);

    int ret = arp_find_entry(ip, (uint8_t*)&e_hdr.dmac);
    if(ret <= 0)
        return ret;

    memcpy(&e_hdr.smac, current_netdev.mac, 6);

    memcpy(skb->data, &e_hdr, ETHER_HDR_LENGTH);
    skb->data += ETHER_HDR_LENGTH;
    
    return 1;
}

uint8_t ethernet_parse(struct sk_buff* skb)
{
    struct ethernet_header* header = (struct ethernet_header*) skb->data;
    header->ethertype = ntohs(header->ethertype);

    skb->hdr.eth = header;
    skb->data += ETHER_HDR_LENGTH;

    uint8_t broadcastmac[] = {255, 255, 255, 255, 255, 255};
    if(memcmp(header->dmac, skb->netdevice->mac, 6) == 0 || memcmp(skb->hdr.eth->dmac, (uint8_t*)&broadcastmac, 6) == 0){
		return 1;
	}

	return 0;
}