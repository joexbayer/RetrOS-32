/**
 * @file udp.c
 * @author Joe Bayer (joexbayer)
 * @brief Basic UDP implementation.
 * @version 0.1
 * @date 2022-06-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/udp.h>
#include <net/ipv4.h>
#include <terminal.h>


int udp_send(struct sk_buff* skb, char* data, uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint32_t length)
{
	struct udp_header hdr;
	hdr.destport = dport;
	hdr.srcport = sport;
	hdr.udp_length = length + sizeof(struct udp_header);
	hdr.checksum = 0;

	if(ip_add_header(skb, dip, UDP, length+sizeof(struct udp_header)) <= 0)
		return -1;

	UDP_HTONS(&hdr);

	memcpy(skb->data, &hdr, sizeof(struct udp_header));
	memcpy(skb->data + sizeof(struct udp_header), (char*) data, length);

	//((struct udp_header*) skb->data)->checksum = transport_checksum(sip, dip, UDP, (char*) skb->data, htons(length+sizeof(struct udp_header)));
	//((struct udp_header*) skb->data)->checksum = htons(((struct udp_header*) skb->data)->checksum);

	skb->len += sizeof(struct udp_header) + length;
	skb->data += sizeof(struct udp_header) + length;
	
	skb->stage = NEW_SKB;
	skb->action = SEND;
	twritef("Creating UDP. size: %d\n", skb->len);

	return 1;
}

int upd_parse(struct sk_buff* skb){

	struct udp_header* hdr = (struct udp_header* ) skb->data;
	skb->hdr.udp = hdr;

	uint16_t udp_checksum = transport_checksum(skb->hdr.ip->saddr, skb->hdr.ip->daddr, UDP, (char*)skb->data, skb->hdr.udp->udp_length);
	if( udp_checksum != 0){
		twriteln("UDP checksum failed.\n");
		return 0;
	}
	skb->data = skb->data + sizeof(struct udp_header);

	UDP_NTOHS(hdr);

	//udp_handle(skb);

    return 1;
}