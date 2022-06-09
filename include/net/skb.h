#ifndef SKB_H
#define SKB_H

#include <stdint.h>
#include <net/netdev.h>
#include <memory.h>
#include <util.h>

enum sk_stage {
    UNUSED,
	NEW_SKB,
	IN_PROGRESS,
	DONE
};

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

    uint8_t stage;
};

#define MAX_SKBUFFERS 255


void init_sk_buffers();
struct sk_buff* get_skb();
struct sk_buff* next_skb();

#define ALLOCATE_SKB(skb)               \
    (skb)->data = alloc(0x600);         \
    (skb)->head = skb->data;            \
    (skb)->tail = skb->head;            \
    (skb)->end = skb->head+0xFFF;       \
    (skb)->stage = NEW_SKB;

#define FREE_SKB(skb)           \
    if((skb)->head != NULL)     \
        free((skb)->head);      \
    (skb)->len = -1;            \
    (skb)->stage = UNUSED;   

#include <net/ethernet.h>
#include <net/arp.h>

#endif // !SKB_H

