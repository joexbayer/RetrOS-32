#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include <net/utils.h>
#include <net/skb.h>

#define ARP_ETHERNET    0x0001
#define ARP_IPV4        0x0800
#define ARP_REQUEST     0x0001
#define ARP_REPLY       0x0002

#define ARP_NTOHS(hdr) \
    (hdr)->hwtype = ntohs((hdr)->hwtype); \
	(hdr)->opcode = ntohs((hdr)->opcode); \
	(hdr)->protype = ntohs((hdr)->protype);

#define ARPC_NTOHL(content) \
    (content)->sip = ntohl((content)->sip); \
	(content)->dip = ntohl((content)->dip);

#define ARP_HTONS(hdr) \
    (hdr)->hwtype = htons((hdr)->hwtype); \
	(hdr)->opcode = htons((hdr)->opcode); \
	(hdr)->protype = htons((hdr)->protype);

#define ARPC_HTONL(content) \
    (content)->sip = htonl((content)->sip); \
	(content)->dip = htonl((content)->dip);  

struct arp_header
{
    uint16_t hwtype;
    uint16_t protype;
    uint8_t hwsize;
    uint8_t prosize;
    uint16_t opcode;
} __attribute__((packed));

struct arp_content
{
    uint8_t smac[6];
    uint32_t sip;
    uint8_t dmac[6];
    uint32_t dip;
} __attribute__((packed));

struct arp_entry
{
	uint8_t smac[6];
	uint32_t sip;
}__attribute__((packed));

uint8_t arp_parse(struct sk_buff* skb);
int arp_find_entry(uint32_t ip, uint8_t* mac);
void init_arp();

/* For testing. */
void arp_request();
int arp_add_entry(struct arp_content* arp);

#define ARP_FILL_HEADER(header, type) \
    header.opcode = type; \
	header.prosize = 4; \
	header.protype = htons(IPV4); \
	header.hwsize = 6; \
	header.hwtype = ARP_ETHERNET;

#endif // !ARP_H