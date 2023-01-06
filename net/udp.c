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
#include <net/socket.h>

#include <serial.h>


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
	return 1;
}

int udp_parse(struct sk_buff* skb){

	struct udp_header* hdr = (struct udp_header* ) skb->data;
	skb->hdr.udp = hdr;

	uint16_t udp_checksum = transport_checksum(skb->hdr.ip->saddr, skb->hdr.ip->daddr, UDP, (uint8_t*)skb->data, skb->hdr.udp->udp_length);
	if( udp_checksum != 0){
		dbgprintf("[UDP] checksum failed %d %x.\n", udp_checksum, udp_checksum);
		/* TODO  UDP CHECKSUM IS BROKEN. */
	}
	skb->data = skb->data + sizeof(struct udp_header);

	UDP_NTOHS(hdr);

	int payload_size = skb->hdr.udp->udp_length-sizeof(struct udp_header);

	int ret = udp_deliver_packet(skb->hdr.ip->daddr, skb->hdr.udp->destport, (char*)skb->data, payload_size);
	if(ret <= 0)
		dbgprintf("[UDP][Warning] socket buffer full!!");
		
	dbgprintf("[UDP] PORT %d -> %d, len: %d.\n", hdr->srcport, hdr->destport, hdr->udp_length);

    return 1;
}