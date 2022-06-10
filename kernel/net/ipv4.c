#include <net/ipv4.h>
#include <net/utils.h>
#include <terminal.h>

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

int parse_ip(struct sk_buff* skb)
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