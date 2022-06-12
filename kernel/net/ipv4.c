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

uint32_t ip_to_int(const char* ip)
{
    uint8_t ipbytes[4];

    int i = 0;
    int8_t j = 3;
    while (ip+i && i<strlen(ip))
    {
       char digit = ip[i];
       if (isdigit(digit) == 0 && digit != '.'){
           return 0;
       }
        j= digit == '.' ? j - 1 : j;
       ipbytes[j]= ipbytes[j]*10 + atoi(&digit);

        i++;
    }

    uint32_t a = ipbytes[0];
    uint32_t b =  ( uint32_t)ipbytes[1] << 8;
    uint32_t c =  ( uint32_t)ipbytes[2] << 16;
    uint32_t d =  ( uint32_t)ipbytes[3] << 24;
    return a+b+c+d;
}