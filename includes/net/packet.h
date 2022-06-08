#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define PACKET_SIZE 2048

struct packet {
    uint8_t buffer[PACKET_SIZE];
    uint16_t size;
};

#endif /* PACKET_H */
