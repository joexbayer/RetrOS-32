#ifndef __IPC_H
#define __IPC_H

#include <stdint.h>

/**
 * Events
 * Messages
 */

struct event {
    uint8_t type;
    uint8_t action;
};

#endif // !__IPC_H