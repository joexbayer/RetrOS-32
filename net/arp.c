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
#include <net/dhcpd.h>
#include <terminal.h>

#define MAX_ARP_ENTRIES 25

struct arp_entry arp_entries[MAX_ARP_ENTRIES];

void init_arp()
{
	for (size_t i = 0; i < MAX_ARP_ENTRIES; i++)
	{
		arp_entries[i].sip = 0;
	}

	uint8_t broadcast_mac[6] = {255, 255, 255, 255, 255, 255};
	
	memcpy(&arp_entries[0].smac, &broadcast_mac, 6);
	arp_entries[0].sip = BROADCAST_IP;
}

/**
 * @brief Adds a arp request / response to the arp cache.
 * 
 * @param arp ARP content packet.
 * @return int 
 */
int arp_add_entry(struct arp_content* arp)
{
	/* Check if ARP entry already exists. */
	for (size_t i = 0; i < MAX_ARP_ENTRIES; i++)
		if(memcmp((uint8_t*)&arp->smac, (uint8_t*)&arp_entries[i].smac, 6))
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

/**
 * @brief Finds a APR entry in the cache based on the IP
 * Result is copied to given MAC pointer.
 * 
 * @param ip IP to search for
 * @param mac buffer to copy MAC into.
 * @return int 
 */
int arp_find_entry(uint32_t ip, uint8_t* mac)
{
	for (size_t i = 0; i < MAX_ARP_ENTRIES; i++){
		if(arp_entries[i].sip == ip){
			memcpy(mac, arp_entries[i].smac, 6);
			return 1;
		}
	}
	return 0;
}

void arp_print_cache()
{
	twriteln("ARP cache:");
	for (size_t i = 0; i < MAX_ARP_ENTRIES; i++)
		if(arp_entries[i].sip != 0){

			uint32_t ip = ntohl(arp_entries[i].sip);
			uint8_t bytes[4];
			bytes[0] = (ip >> 24) & 0xFF;
			bytes[1] = (ip >> 16) & 0xFF;
			bytes[2] = (ip >> 8) & 0xFF;
			bytes[3] = ip & 0xFF; 
			twritef("(%d.%d.%d.%d) at %x:%x:%x:%x:%x:%x\n",
					bytes[3], bytes[2], bytes[1], bytes[0],
					arp_entries[i].smac[0], arp_entries[i].smac[1], arp_entries[i].smac[2], arp_entries[i].smac[3], arp_entries[i].smac[4], arp_entries[i].smac[5]);
		}
}

/**
 * @brief Helper method that adds ethernet header and send ARP packet.
 * 
 * @param content ARP content struct
 * @param hdr ARP header
 * @param skb buffer to send.
 */
void __arp_send(struct arp_content* content, struct arp_header* hdr, struct sk_buff* skb)
{
	skb->proto = ARP;
	twriteln("Creating Ethernet header.");
	int ret = ethernet_add_header(skb, content->dip);
	if(ret <= 0){
		twriteln("Error adding ethernet header");
		return;
	}

	ARP_HTONS(hdr);
	ARPC_HTONL(content);

	memcpy(skb->data, hdr, sizeof(struct arp_header));
	skb->data += sizeof(struct arp_header);
	skb->len += sizeof(struct arp_header);

	memcpy(skb->data, content, sizeof(struct arp_content));
	skb->data += sizeof(struct arp_content);
	skb->len += sizeof(struct arp_content);

	twritef("Creating ARP Response. size: %d \n", skb->len);
	skb->stage = NEW_SKB;
	skb->action = SEND;
}

/**
 * @brief Create a ARP response packet based on request content.
 * 
 * @param content APR request content.
 */
void arp_respond(struct arp_content* content)
{
	if(dhcp_get_ip() == -1)
		return;

	struct sk_buff* skb = get_skb();
    ALLOCATE_SKB(skb);
    skb->stage = IN_PROGRESS;

	struct arp_header a_hdr;
	ARP_FILL_HEADER(a_hdr, ARP_REPLY);

	memcpy(&content->dmac, &content->smac, 6);
	memcpy(&content->smac, &current_netdev.mac, 6);
	content->dip = content->sip;
	content->sip = dhcp_get_ip();

	__arp_send(content, &a_hdr, skb);
}

/**
 * @brief Create a ARP request packet and send it.
 * 
 */
void arp_request()
{
	if(dhcp_get_ip() == -1)
		return;

	twriteln("Creating ARP packet.");
	struct sk_buff* skb = get_skb();
    ALLOCATE_SKB(skb);
    skb->stage = IN_PROGRESS;

	struct arp_header a_hdr;
	struct arp_content a_content;

	ARP_FILL_HEADER(a_hdr, ARP_REQUEST);

	uint8_t broadcast_mac[6] = {255, 255, 255, 255, 255, 255};

	a_content.dip = BROADCAST_IP;
	memcpy(a_content.smac, current_netdev.mac, 6);
	a_content.sip = dhcp_get_ip();
	memcpy(a_content.dmac, broadcast_mac, 6);

	__arp_send(&a_content, &a_hdr, skb);
}

/**
 * @brief Handles ARP packets
 * 
 * @param skb socket buffer to parse
 */
uint8_t arp_parse(struct sk_buff* skb)
{
	struct arp_header* a_hdr = (struct arp_header*) skb->data;
	skb->hdr.arp = a_hdr;
	skb->data = skb->data + sizeof(struct arp_header);

	ARP_NTOHS(a_hdr);

	if(a_hdr->hwtype != ARP_ETHERNET || a_hdr->protype != ARP_IPV4){	
		return 0;
	}

	struct arp_content* arp_content = (struct arp_content*) skb->data;
	ARPC_NTOHL(arp_content);

	twriteln("Received ARP!");

	switch (a_hdr->opcode)
	{
	case ARP_REQUEST:

		arp_add_entry(arp_content);
		arp_respond(arp_content);
		break;
	case ARP_REPLY:
		arp_add_entry(arp_content);
		/* Signal ARP reply was recieved, check for waiting SKBs*/
		break;
	
	default:
		break;
	}

	return 1;
}