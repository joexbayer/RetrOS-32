#ifndef SKB_H
#define SKB_H

#include <stdint.h>
#include <net/ethernet.h>
#include <net/netdev.h>
#include <memory.h>

struct sk_buff {

    struct netdev* netdevice;

    union {
        struct ethernet_header* eth;
    } header;

    uint16_t len;
    uint16_t data_len;

    uint8_t* head;
    uint8_t* tail;
    uint8_t* data;
    uint8_t* end;

};

#define ALLOCATE_SKB(buffer, skb)   \
    skb->data = alloc(0x1000);      \
    skb->head = skb->data;          \
    skb->tail = skb->head;          \
    skb->end = skb->head+0xFFF;     \


#endif // !SKB_H
