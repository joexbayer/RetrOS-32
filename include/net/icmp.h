#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>
#include <net/skb.h>

#define ICMP_REPLY 0x00
#define ICMP_V4_ECHO 0x08

struct icmp {
    uint8_t type;
    uint8_t code;
    uint16_t csum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));


#define ICMP_HTONS(pkt) \
    (pkt)->csum = htons((pkt)->csum); \
    (pkt)->id = htons((pkt)->id); \
    (pkt)->sequence = htons((pkt)->sequence);

#define ICMP_NTOHS(pkt) \
    (pkt)->csum = ntohs((pkt)->csum); \
    (pkt)->id = ntohs((pkt)->id); \
    (pkt)->sequence = ntohs((pkt)->sequence);

int icmp_parse(struct sk_buff* skb);
void ping(char* hostname);

#endif /* ICMP_H */
