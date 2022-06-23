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
#include <terminal.h>

int upd_parse(struct sk_buff* skb){

	struct udp_header* hdr = (struct udp_header* ) skb->data;
	skb->hdr.udp = hdr;

	uint16_t udp_checksum = transport_checksum(skb->hdr.ip->saddr, skb->hdr.ip->daddr, UDP, skb->data, skb->hdr.udp->udp_length);
	if( udp_checksum != 0){
		twriteln("UDP checksum failed.\n");
		return 0;
	}
	skb->data = skb->data + sizeof(struct udp_header);

	UDP_NTOHS(hdr);

	//udp_handle(skb);

    return 1;
}