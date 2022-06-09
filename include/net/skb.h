#ifndef SKB_H
#define SKB_H

#include <stdint.h>
#include <net/netdev.h>
#include <memory.h>
#include <util.h>

struct sk_buff {

    struct netdev* netdevice;

    union {
        struct ethernet_header* eth;
        struct arp_header* arp;
    } hdr;

    int16_t len;
    uint16_t data_len;

    uint8_t* head;
    uint8_t* tail;
    uint8_t* data;
    uint8_t* end;
};

#define MAX_SKBUFFERS 255


void init_sk_buffers();
struct sk_buff* get_skb();

#define ALLOCATE_SKB(skb)            \
    (skb)->data = alloc(0x600);       \
    (skb)->head = skb->data;           \
    (skb)->tail = skb->head;           \
    (skb)->end = skb->head+0xFFF;      

#define FREE_SKB(skb)   \
    free((skb)->data);    \
    (skb)->len = -1;      \
    (skb)->data = NULL;   

#include <net/ethernet.h>
#include <net/arp.h>

#endif // !SKB_H

