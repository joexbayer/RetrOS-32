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
#include <net/dhcp.h>
#include <net/net.h>
#include <terminal.h>
#include <serial.h>

#define MAX_ARP_ENTRIES 25

static struct arp_entry arp_entry_table[MAX_ARP_ENTRIES];

void init_arp()
{
	for (int i = 0; i < MAX_ARP_ENTRIES; i++)
		arp_entry_table[i].sip = 0;

	/*  Add broadcast arp entry */
	uint8_t broadcast_mac[6] = {255, 255, 255, 255, 255, 255};
	memcpy(&arp_entry_table[0].smac, &broadcast_mac, 6);
	arp_entry_table[0].sip = BROADCAST_IP;
}

/**
 * @brief Adds a arp request / response to the arp cache.
 * 
 * @param arp ARP content packet.
 * @return int 
 */
int net_arp_add_entry(struct arp_content* arp)
{
	dbgprintf("Adding %i to arp entries\n", arp->sip);
	/* Check if ARP entry already exists. */
	for (int i = 0; i < MAX_ARP_ENTRIES; i++)
		if(memcmp((uint8_t*)&arp->smac, (uint8_t*)&arp_entry_table[i].smac, 6) == 0)
			return 1;

	for (int i = 0; i < MAX_ARP_ENTRIES; i++){
		if(arp_entry_table[i].sip == 0){
			memcpy((uint8_t*)&arp_entry_table[i].smac, (uint8_t*)&arp->smac, 6);
			arp_entry_table[i].sip = arp->sip;
			dbgprintf("Added APR entry.\n");
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
int net_arp_find_entry(uint32_t ip, uint8_t* mac)
{
	for (int i = 0; i < MAX_ARP_ENTRIES; i++){
		if(arp_entry_table[i].sip == ip){
			memcpy(mac, arp_entry_table[i].smac, 6);
			return 1;
		}
	}
	dbgprintf("Warning: Could not find arp for %i\n", ip);
	return -1;
}

/**
 * @brief Helper method that adds ethernet header and send ARP packet.
 * 
 * @param content ARP content struct
 * @param hdr ARP header
 * @param skb buffer to send.
 */
static void __net_arp_send(struct arp_content* content, struct arp_header* hdr)
{
	struct sk_buff* skb = skb_new();

	skb->proto = ARP;
	int ret = net_ethernet_add_header(skb, content->dip);
	if(ret <= 0){
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

	net_send_skb(skb);
	
}

/**
 * @brief Create a ARP response packet based on request content.
 * 
 * @param content APR request content.
 */
void net_arp_respond(struct arp_content* content)
{
	if(dhcp_get_ip() == -1)
		return;

	struct arp_header a_hdr;
	ARP_FILL_HEADER(a_hdr, ARP_REPLY);

	memcpy(&content->dmac, &content->smac, 6);
	memcpy(&content->smac, &current_netdev.mac, 6);
	content->dip = content->sip;
	content->sip = dhcp_get_ip();

	__net_arp_send(content, &a_hdr);
}

/**
 * @brief Create a ARP request packet and send it.
 * 
 */
void net_net_arp_request()
{
	if(dhcp_get_ip() == -1)
		return;

	struct arp_header a_hdr;
	struct arp_content a_content;

	ARP_FILL_HEADER(a_hdr, ARP_REQUEST);

	uint8_t broadcast_mac[6] = {255, 255, 255, 255, 255, 255};

	a_content.dip = BROADCAST_IP;
	memcpy(a_content.smac, current_netdev.mac, 6);
	a_content.sip = dhcp_get_ip();
	memcpy(a_content.dmac, broadcast_mac, 6);

	__net_arp_send(&a_content, &a_hdr);
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
		return -1;
	}

	struct arp_content* arp_content = (struct arp_content*) skb->data;
	ARPC_NTOHL(arp_content);

	switch (a_hdr->opcode)
	{
	case ARP_REQUEST:

		net_arp_add_entry(arp_content);
		net_arp_respond(arp_content);
		break;
	case ARP_REPLY:
		net_arp_add_entry(arp_content);
		/* Signal ARP reply was recieved, check for waiting SKBs*/
		break;
	
	default:
		break;
	}

	return 0;
}