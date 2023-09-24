#include <ipc.h>
#include <memory.h>

/* Initializes an IPC channel with the given buffer size */
struct ipc_channel* ipc_channel_init(int size) {
    struct ipc_channel* channel = (struct ipc_channel*) kalloc(sizeof(struct ipc_channel));
    if (!channel) {
        return NULL;
    }

    channel->rbuf = rbuffer_new(size);
    if (!channel->rbuf) {
        kfree(channel);
        return NULL;
    }
    
    return channel;
}

/* Sends a message over the IPC channel */
error_t ipc_send(struct ipc_channel* channel, struct ipc_message* message) {
    if (!channel || !message || !message->data || message->length <= 0) {
        return -ERROR_NULL_POINTER;
    }
    return channel->rbuf->ops->add(channel->rbuf, message->data, message->length);
}

/* Receives a message from the IPC channel */
error_t ipc_receive(struct ipc_channel* channel, struct ipc_message* message) {
    if (!channel || !message || !message->data || message->length <= 0) {
        return -ERROR_NULL_POINTER;
    }
    return channel->rbuf->ops->read(channel->rbuf, message->data, message->length);
}

/* Cleans up and releases the IPC channel */
void ipc_channel_free(struct ipc_channel* channel) {
    if (channel) {
        rbuffer_free(channel->rbuf);
        kfree(channel);
    }
}
