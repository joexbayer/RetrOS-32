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

/* Initializes an IPC channel with the given buffer size */
struct ipc_channel* ipc_channel_init(int size);

/* Sends a message over the IPC channel */
error_t ipc_send(struct ipc_channel* channel, ipc_message_t* message);

/* Receives a message from the IPC channel */
error_t ipc_receive(struct ipc_channel* channel, ipc_message_t* message);

/* Cleans up and releases the IPC channel */
void ipc_channel_free(struct ipc_channel* channel);

#endif /* IPC_INTERFACE_H */