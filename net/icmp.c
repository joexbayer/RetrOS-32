/**
 * @file icmp.c
 * @author Joe Bayer (joexbayer)
 * @brief ICMP implementation.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <net/icmp.h>
#include <net/net.h>
#include <serial.h>
#include <net/skb.h>
#include <net/dns.h>

void net_icmp_handle(struct sk_buff* skb)
{
    if(skb->hdr.icmp->type != ICMP_V4_ECHO)
        return;
    dbgprintf("Ping reply from %i: icmp_seq= %d ttl=64\n", skb->hdr.ip->saddr, skb->hdr.icmp->sequence/256);

    skb->hdr.icmp->type = ICMP_REPLY;
    skb->hdr.icmp->csum = 0;
    skb->hdr.icmp->csum = checksum(skb->hdr.icmp, skb->len, 0);

    struct icmp response;
    memcpy(&response, &skb->hdr.icmp, sizeof(struct icmp));

    struct sk_buff* _skb = skb_new();

    if(net_ipv4_add_header(_skb, skb->hdr.ip->saddr, ICMPV4, sizeof(struct icmp)) < 0){
        skb_free(_skb);
	    return;
    }

    memcpy(_skb->data, &response, sizeof(struct icmp));

	_skb->len += sizeof(struct icmp);
	_skb->data += sizeof(struct icmp);
	
	net_send_skb(_skb);
}  

int net_icmp_response()
{
    return 1;
}

int net_icmp_request(uint32_t ip)
{
    struct sk_buff* skb = skb_new();

    struct icmp ping = {
        .type = ICMP_V4_ECHO,
        .code = 0,
        .id = 0,
        .sequence = 0,
        .csum = 0
    };

    if(net_ipv4_add_header(skb, ip, ICMPV4, sizeof(struct icmp)) < 0){
        skb_free(skb);
		return -1;
    }

    ICMP_NTOHS(&ping);
    ping.csum = checksum(&ping, sizeof(struct icmp), 0);

    memcpy(skb->data, &ping, sizeof(struct icmp));

	skb->len += sizeof(struct icmp);
	skb->data += sizeof(struct icmp);
	
	net_send_skb(skb);

    return 0;
}

void ping(char* hostname)
{   
    uint32_t ip;
    if(!isdigit(hostname[0])){
        ip = gethostname(hostname);
        if(ip <= 0){
            return;
        }
    } else {
        ip = htonl(ip_to_int(hostname));
    }

    net_icmp_request(htonl(ip));
}

int net_icmp_parse(struct sk_buff* skb)
{
    struct icmp* icmp_hdr = (struct icmp * ) skb->data;
    skb->hdr.icmp = icmp_hdr;
    skb->data = skb->data + sizeof(struct icmp);

    // calculate checksum, should be 0.
    uint16_t csum_icmp = checksum(icmp_hdr, skb->len, 0);
    if( 0 != csum_icmp){
        return -1;
    }
    ICMP_HTONS(icmp_hdr);

    return 0;
}