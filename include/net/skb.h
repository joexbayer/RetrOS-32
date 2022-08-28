#ifndef SKB_H
#define SKB_H

#include <stdint.h>
#include <net/netdev.h>
#include <memory.h>
#include <net/utils.h>
#include <util.h>

enum sk_stage {
    UNUSED,
	NEW_SKB,
	IN_PROGRESS,
	DONE
};

enum sk_action {
    SEND,
    RECIEVE
};

struct sk_buff {

    struct netdev* netdevice;

    struct {
        struct ethernet_header* eth;
        struct arp_header* arp;
        struct ip_header* ip;
        /* Can TCP and UDP be a union? */
        struct udp_header* udp;
        struct tcp_header* tcp;
        struct icmp* icmp;
    } hdr;

    int16_t len;
    uint16_t data_len;
    uint16_t proto;

    uint8_t* head;
    uint8_t* tail;
    uint8_t* data;
    uint8_t* end;

    uint8_t stage;
    uint8_t action;
};

#define MAX_SKBUFFERS 0x200

void init_sk_buffers();
struct sk_buff* get_skb();
struct sk_buff* next_skb();

#define ALLOCATE_SKB(skb)               \
    (skb)->data = alloc(0x600);         \
    memset((skb)->data, 0, 0x600);       \
    (skb)->head = skb->data;            \
    (skb)->tail = skb->head;            \
    (skb)->end = skb->head+0xFFF;       \
    (skb)->len = 0;                     \
    (skb)->stage = NEW_SKB;

#define FREE_SKB(skb)           \
    if((skb)->head != NULL)     \
        free((skb)->head);      \
    (skb)->len = -1;            \
    (skb)->stage = UNUSED;      \
    (skb)->head = NULL;

#include <net/ethernet.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>

#endif // !SKB_H

