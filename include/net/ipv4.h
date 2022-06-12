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

int ip_parse(struct sk_buff* skb);
uint32_t ip_to_int(const char * ip);

#endif