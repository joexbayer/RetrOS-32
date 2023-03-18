#ifndef __NET_H
#define __NET_H

#include <net/skb.h>

void net_incoming_packet_handler();
void net_send_skb(struct sk_buff* skb);

#endif /* __NET_H */
