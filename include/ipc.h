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

enum msg_box_statues {
    MSG_UNUSED,
    MSG_USED
};

struct message {
    void* data;
    uint16_t len;
};

#define MESSAGE_BOX_MAX_SIZE 10
struct message_box {
    unsigned char id;
    struct message messages[MESSAGE_BOX_MAX_SIZE];
    unsigned short head;
    unsigned short tail;

    unsigned char status;
};

void ipc_msg_box_init();

void signal(int signal);
void signal_wait(int signal);

#endif // !__IPC_H