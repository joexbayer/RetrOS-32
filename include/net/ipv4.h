#ifndef IPV4_H
#define IPV4_H

#include <stdint.h>
#include <net/skb.h>

struct ip_header {
    uint8_t ihl : 4;
    uint8_t version : 4;
    uint8_t tos;
    uint16_t len;
    uint16_t id;
    uint16_t frag_offset;
    uint8_t ttl;
    uint8_t proto;
    uint16_t csum;
    uint32_t saddr;
    uint32_t daddr;
} __attribute__((packed));

#define IPV4 0x04
#define ICMPV4 0x01
#define UDP 0x11

#define BROADCAST_IP 4294967295


#define IP_HEADER_CREATE(hdr, proto, length) \
    hdr.version = IPV4; \
    hdr.ihl = 0x05; \
    hdr.tos = 0; \
    hdr.len = length+hdr.ihl*4; \
    hdr.frag_offset = 0x4000; \
    hdr.ttl = 64; \
    hdr.proto = proto;

#define IP_HTONL(ihdr)              \
    (ihdr)->len = htons((ihdr)->len);   \
    (ihdr)->id = htons((ihdr)->id);     \
    (ihdr)->daddr = htonl((ihdr)->daddr);   \
    (ihdr)->saddr = (ihdr)->saddr;  \
    (ihdr)->csum = htons((ihdr)->csum); \
    (ihdr)->frag_offset = htons((ihdr)->frag_offset);

#define IP_NTOHL(ihdr) \
    (ihdr)->saddr = ntohl((ihdr)->saddr); \
    (ihdr)->daddr = ntohl((ihdr)->daddr); \
    (ihdr)->len = ntohs((ihdr)->len); \
    (ihdr)->id = ntohs((ihdr)->id);

int ip_add_header(struct sk_buff* skb, uint32_t ip, uint8_t proto, uint32_t length);
int ip_parse(struct sk_buff* skb);



#endif