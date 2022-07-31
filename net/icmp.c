#include <net/icmp.h>
#include <net/skb.h>
#include <screen.h>
#include <terminal.h>
#include <net/dns.h>


void icmp_print(struct sk_buff* skb)
{
    unsigned char bytes[4];
    bytes[3] = (skb->hdr.ip->saddr >> 24) & 0xFF;
    bytes[2] = (skb->hdr.ip->saddr >> 16) & 0xFF;
    bytes[1] = (skb->hdr.ip->saddr >> 8) & 0xFF;
    bytes[0] = skb->hdr.ip->saddr & 0xFF;  

    twritef("ICMP: %d from to %d.%d.%d.%d: icmp_seq= %d ttl=64 protocol: IPv4\n", skb->hdr.ip->len - skb->hdr.ip->ihl*4, bytes[3], bytes[2], bytes[1], bytes[0], skb->hdr.icmp->sequence/256);
}

void icmp_handle(struct sk_buff* skb)
{

    if(skb->hdr.icmp->type == ICMP_REPLY)
        return;
    // ICMP reply setup
    skb->hdr.icmp->type = ICMP_REPLY;
    skb->hdr.icmp->csum = 0;
    skb->hdr.icmp->csum = checksum(skb->hdr.icmp, skb->len, 0);

    icmp_print(skb);

    struct icmp response;
    memcpy(&response, &skb->hdr.icmp, sizeof(struct icmp));

    skb->proto = ICMPV4;
}  

int icmp_response()
{
    return 1;
}

int icmp_request(uint32_t ip)
{
    struct sk_buff* skb = get_skb();
    ALLOCATE_SKB(skb);
    skb->stage = IN_PROGRESS;

    struct icmp ping;
    ping.type = ICMP_V4_ECHO;
    ping.code = 0;
    ping.id = 0;
    ping.sequence = 0;
    ping.csum = 0;

    if(ip_add_header(skb, ip, ICMPV4, sizeof(struct icmp)) <= 0)
		return -1;

    ICMP_NTOHS(&ping);
    ping.csum = checksum(&ping, sizeof(struct icmp), 0);

    memcpy(skb->data, &ping, sizeof(struct icmp));

	skb->len += sizeof(struct icmp);
	skb->data += sizeof(struct icmp);
	
	skb->stage = NEW_SKB;
	skb->action = SEND;

    return 1;
}

void ping(char* hostname)
{   
    uint32_t ip;
    if(!isdigit(hostname[0])){
        ip = gethostname(hostname);
        if(ip <= 0){
            twriteln("ICMP: Cannot resolve hostname.");
            return;
        }
    } else {
        ip = htonl(ip_to_int(hostname));
    }

    icmp_request(htonl(ip));
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

    ICMP_HTONS(icmp_hdr);

    icmp_handle(skb);

    return 1;
}