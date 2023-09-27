#ifndef IPC_INTERFACE_H
#define IPC_INTERFACE_H


#include <stdint.h>
#include <rbuffer.h>

/* IPC Message structure */
struct ipc_message {
    unsigned char* data; // Pointer to the message data
    int length;          // Length of the message
};

/* IPC Channel structure */
struct ipc_channel {
    struct ring_buffer* rbuf; // Ring buffer for the IPC channel
};

int sys_ipc_open();
int sys_ipc_close(int channel);
int sys_ipc_send(int channel, void* data, int length);
int sys_ipc_send(int channel, void* data, int length);
int sys_ipc_receive(int channel, void* data, int length);

#endif /* IPC_INTERFACE_H */