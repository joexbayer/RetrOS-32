#ifndef UDP_H
#define UDP_H

#include <net/utils.h>
#include <net/skb.h>

#define UDP_NTOHS(hdr) \
    (hdr)->srcport = ntohs((hdr)->srcport); \
	(hdr)->destport = ntohs((hdr)->destport); \
	(hdr)->udp_length = ntohs((hdr)->udp_length); \
	(hdr)->checksum = ntohs((hdr)->checksum);

#define UDP_HTONS(hdr) \
    (hdr)->srcport = htons((hdr)->srcport); \
	(hdr)->destport = htons((hdr)->destport); \
	(hdr)->udp_length = htons((hdr)->udp_length); \
	(hdr)->checksum = htons((hdr)->checksum);

struct udp_header
{
	uint16_t srcport;
	uint16_t destport;
	uint16_t udp_length;
	uint16_t checksum;
};

int udp_parse(struct sk_buff* skb);

#endif /* UDP_H */
