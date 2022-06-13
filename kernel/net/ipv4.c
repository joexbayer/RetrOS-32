#include <net/ipv4.h>
#include <net/utils.h>
#include <terminal.h>
#include <util.h>

void __ip_ntohl(struct ip_header *hdr)
{
    hdr->saddr = ntohl(hdr->saddr);
    hdr->daddr = ntohl(hdr->daddr);
    hdr->len = ntohs(hdr->len);
    hdr->id = ntohs(hdr->id);
}

void __ip_htonl(struct ip_header* ihdr){
    ihdr->len = htons(ihdr->len);
    ihdr->id = htons(ihdr->id);
    ihdr->daddr = htonl(ihdr->daddr);
    ihdr->saddr = ihdr->saddr;
    ihdr->csum = htons(ihdr->csum);
    ihdr->frag_offset = htons(ihdr->frag_offset);
}

void __ip_send(struct ip_header* ihdr, struct sk_buff* skb, uint32_t dip)
{
    skb->len += ihdr->ihl * 4;
    skb->proto = IP;
	twriteln("Creating Ethernet header.");
	int ret = ethernet_add_header(skb, dip);
	if(ret <= 0){
		twriteln("Error adding ethernet header");
		return;
	}

    memcpy(skb->data, ihdr, sizeof(struct ip_header));
    skb->len += sizeof(struct ip_header);

    twritef("Creating IP Packet. size: %d \n", skb->len);
	skb->stage = NEW_SKB;
	skb->action = SEND;
}

int ip_parse(struct sk_buff* skb)
{
    struct ip_header* hdr = (struct ip_header* ) skb->data;
    skb->hdr.ip = hdr;

    uint16_t csum = checksum(hdr, hdr->ihl * 4, 0);
    if(0 != csum){
        twriteln("Checksum failed (IPv4)");
        return 0;
    }

    __ip_ntohl(hdr);
    skb->data = skb->data+(skb->hdr.ip->ihl*4);

    /* TODO: Check ip for destination or broadcast */

    return 1;
}

void print_ip(uint32_t ip)
{
    unsigned char bytes[4];
    bytes[0] = (ip >> 24) & 0xFF;
    bytes[1] = (ip >> 16) & 0xFF;
    bytes[2] = (ip >> 8) & 0xFF;
    bytes[3] = ip & 0xFF;   
    twritef("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]); 
}

/*  Function from: https://www.lemoda.net/c/ip-to-integer/ */
uint32_t ip_to_int (const char * ip)
{
    unsigned v = 0;
    int i;
    const char * start;

    start = ip;
    for (i = 0; i < 4; i++) {
        char c;
        int n = 0;
        while (1) {
            c = * start;
            start++;
            if (c >= '0' && c <= '9') {
                n *= 10;
                n += c - '0';
            } else if ((i < 3 && c == '.') || i == 3) {
                break;
            } else {
                return 0;
            }
        }
        if (n >= 256) {
            return 0;
        }
        v *= 256;
        v += n;
    }
    return v;
}