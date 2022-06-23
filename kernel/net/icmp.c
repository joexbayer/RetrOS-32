#include <net/icmp.h>
#include <net/skb.h>
#include <screen.h>
#include <terminal.h>


void icmp_print(struct sk_buff* skb)
{
    unsigned char bytes[4];
    bytes[3] = (skb->hdr.ip->saddr >> 24) & 0xFF;
    bytes[2] = (skb->hdr.ip->saddr >> 16) & 0xFF;
    bytes[1] = (skb->hdr.ip->saddr >> 8) & 0xFF;
    bytes[0] = skb->hdr.ip->saddr & 0xFF;  

    scrprintf(0, 5, "ICMP : %d bytes to %d.%d.%d.%d: icmp_seq= %d ttl=64 protocol: IPv4\n", skb->hdr.ip->len - skb->hdr.ip->ihl*4, bytes[3], bytes[2], bytes[1], bytes[0], skb->hdr.icmp->sequence/256);
}

void icmp_handle(struct sk_buff* skb)
{

    // ICMP reply setup
    skb->hdr.icmp->type = ICMP_REPLY;
    skb->hdr.icmp->csum = 0;
    skb->hdr.icmp->csum = checksum(skb->hdr.icmp, skb->len, 0);

    icmp_print(skb);

    skb->proto = ICMPV4;
}  

int icmp_response()
{
    return 1;
}

int icmp_request()
{
    return 1;
}

int icmp_parse(struct sk_buff* skb)
{
    struct icmp* icmp_hdr = (struct icmp * ) skb->data;
    skb->hdr.icmp = icmp_hdr;
    skb->data = skb->data + sizeof(struct icmp);

    // calculate checksum, should be 0.
    uint16_t csum_icmp = checksum(icmp_hdr, skb->len, 0);
    if( 0 != csum_icmp){
        twriteln("Checksum failed (ICMP)");
        return 0;
    }

    icmp_handle(skb);

    return 1;
}