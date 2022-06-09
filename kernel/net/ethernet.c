#include <net/ethernet.h>
#include <screen.h>
#include <terminal.h>

void print_ethernet(struct ethernet_header* hdr){
    scrprintf(1, 1, "dmac: %x:%x:%x:%x:%x:%x", hdr->dmac[0], hdr->dmac[1], hdr->dmac[2], hdr->dmac[3], hdr->dmac[4], hdr->dmac[5]);
    scrprintf(1, 2, "smac: %x:%x:%x:%x:%x:%x", hdr->smac[0], hdr->smac[1], hdr->smac[2], hdr->smac[3], hdr->smac[4], hdr->smac[5]);
    scrprintf(1, 3, "type: %x", hdr->ethertype);
}

void parse_ethernet(struct sk_buff* skb)
{
    struct ethernet_header* header = (struct ethernet_header*) skb->data;
    header->ethertype = ntohs(header->ethertype);
    print_ethernet(header);

    skb->hdr.eth = header;
    skb->data += ETHER_HDR_LENGTH;

    uint8_t broadcastmac[] = {255, 255, 255, 255, 255, 255};
	if(memcmp(skb->hdr.eth->dmac, skb->netdevice->mac, 6) || memcmp(skb->hdr.eth->dmac, (uint8_t*)&broadcastmac, 6)){

		switch(skb->hdr.eth->ethertype){
			case IP:
				//ip_parse(skb);
				break;

			case ARP:
				//arp_parse(skb);
                twriteln("Recieved ARP packet.");
				return;

			default:
				twriteln("Unknown layer 3 protocol. Dropped.\n");
				free(skb->head);
				return;
		}
	}
}