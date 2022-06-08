#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

#define ETHER_HDR_LENGTH 14
#define MAC_SIZE 6

#define IP 0x0800
#define ARP 0x0806
#define RARP 0x8035

struct ethernet_header
{
    uint8_t  dmac[MAC_SIZE];
    uint8_t  smac[MAC_SIZE];
    uint16_t ethertype;
} __attribute__((packed));

void print_ethernet(struct ethernet_header* hdr);


#endif /* ETHERNET_H */
