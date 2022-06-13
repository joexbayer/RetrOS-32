#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>

#define ICMP_REPLY 0x00
#define ICMP_V4_ECHO 0x08

struct icmp {
    uint8_t type;
    uint8_t code;
    uint16_t csum;
    uint16_t id;
    uint16_t sequence;
} __attribute__((packed));

#endif /* ICMP_H */
