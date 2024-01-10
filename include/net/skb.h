#ifndef SKB_H
#define SKB_H

struct sk_buff;
#include <stdint.h>
#include <net/netdev.h>
#include <memory.h>
#include <net/utils.h>
#include <libc.h>
#include <net/ethernet.h>
#include <net/interface.h>

struct sk_buff {
    struct sk_buff* next;
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

    struct net_interface* interface;
};

struct skb_queue;
struct skb_queue_operations {
	/*Adds a new network packet to the end of the packet queue.*/
	int (*add)(struct skb_queue* skb_queue, struct sk_buff* skb);
	/*Removes and returns the first network packet from the packet queue.*/
	struct sk_buff* (*remove)(struct skb_queue* skb_queue);
};
struct skb_queue {
	mutex_t lock;
	struct sk_buff* _head;
	struct sk_buff* _tail;

	struct skb_queue_operations* ops;

	int size; /***/
};

#define SKB_QUEUE_READY(queue) queue->size > 0

struct skb_queue* skb_new_queue();
void skb_free_queue(struct skb_queue* queue);

struct sk_buff* skb_new();
void skb_free(struct sk_buff* skb);

#define ALLOCATE_SKB(skb)               \
    (skb)->data = kalloc(0x600);         \
    memset((skb)->data, 0, 0x600);       \
    (skb)->head = skb->data;            \
    (skb)->tail = skb->head;            \
    (skb)->end = skb->head+0x600;       \
    (skb)->len = 0;

#define FREE_SKB(skb)           \
    if((skb)->head != NULL)     \
        kfree((skb)->head);      \
    (skb)->len = -1;            \
    (skb)->head = NULL;

#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>

#endif // !SKB_H

