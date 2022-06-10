/**
 * @file arp.c
 * @author Joe Bayer (joexbayer)
 * @brief Handles ARP parsing, caching and lookup.
 * @version 0.1
 * @date 2022-06-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/arp.h>
#include <terminal.h>

#define MAX_ARP_ENTRIES 25

struct arp_entry arp_entries[MAX_ARP_ENTRIES];

void init_arp()
{
	for (size_t i = 0; i < MAX_ARP_ENTRIES; i++)
	{
		arp_entries[i].sip = 0;
	}
}

int arp_add_entry(struct arp_content* arp)
{
	/* Check if ARP entry already exists. */
	for (size_t i = 0; i < MAX_ARP_ENTRIES; i++)
		if(arp_entries[i].sip == arp->sip)
			return 1;

	for (size_t i = 0; i < MAX_ARP_ENTRIES; i++)
	{
		if(arp_entries[i].sip == 0)
		{
			memcpy((uint8_t*)&arp_entries[i].smac, (uint8_t*)&arp->smac, 6);
			arp_entries[i].sip = arp->sip;
			twriteln("Added APR entry.");
			return 1;
		}
	}

	return 0;
}

int arp_find_entry(uint32_t ip, uint8_t* mac)
{
	for (size_t i = 0; i < MAX_ARP_ENTRIES; i++)
		if(arp_entries[i].sip == ip){
			memcpy(mac, arp_entries[i].smac, 6);
			return 1;
		}
	
	return 0;
}

void __arp_ntohs(struct arp_header* a_hdr){

	a_hdr->hwtype = ntohs(a_hdr->hwtype);
	a_hdr->opcode = ntohs(a_hdr->opcode);
	a_hdr->protype = ntohs(a_hdr->protype);
}

void __arp_content_ntohs(struct arp_content* content)
{

	content->sip = ntohl(content->sip);
	content->dip = ntohl(content->dip);
}

void __arp_content_htons(struct arp_content* content)
{

	content->sip = htonl(content->sip);
	content->dip = htonl(content->dip);
}

void __arp_htons(struct arp_header* a_hdr)
{
	a_hdr->hwtype = htons(a_hdr->hwtype);
	a_hdr->opcode = htons(a_hdr->opcode);
	a_hdr->protype = htons(a_hdr->protype);
}

/**
 * @brief Handles ARP packets
 * 
 * @param skb socket buffer to parse
 */
uint8_t parse_arp(struct sk_buff* skb)
{
	struct arp_header* a_hdr = (struct arp_header*) skb->data;
	skb->hdr.arp = a_hdr;
	skb->data = skb->data + sizeof(struct arp_header);

	__arp_ntohs(a_hdr);

	if(a_hdr->opcode != ARP_REQUEST || a_hdr->hwtype != ARP_ETHERNET || a_hdr->protype != ARP_IPV4){	
		return 0;
	}

	struct arp_content* arp_content = (struct arp_content*) skb->data;
	__arp_content_ntohs(arp_content);

	int ret = arp_add_entry(arp_content);
	if(!ret)
		return ret;
	

	return 0;
}