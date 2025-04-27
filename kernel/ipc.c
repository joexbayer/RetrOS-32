/**
 * @file ipc.c
 * @author Joe Bayer (joexbayer)
 * @brief Inter process communication.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <ipc.h>
#include <memory.h>
#include <syscalls.h>
#include <syscall_helper.h>

#define IPC_MAX_CHANNELS 16
#define IPC_MAX_SIZE 1024

#define IPC_VALID_CHANNEL(channel) if(channel < 0 || channel >= IPC_MAX_CHANNELS || channels[channel].rbuf == NULL) {return -1;}

/* handle to channel implementation */
static struct ipc_channel channels[IPC_MAX_CHANNELS] = {0};

static int __ipc_alloc_channel() {
    for (int i = 0; i < IPC_MAX_CHANNELS; i++)
        if (!channels[i].rbuf)
            return i;
    return -1;
}

/* userspace interface */
int sys_ipc_open()
{
    int channel = __ipc_alloc_channel();
    if (channel < 0) {
        return -1;
    }

    channels[channel].rbuf = rbuffer_new(IPC_MAX_SIZE);
    return channel;
}
EXPORT_SYSCALL(SYSCALL_IPC_OPEN, sys_ipc_open);


int sys_ipc_close(int channel)
{
    IPC_VALID_CHANNEL(channel);

    rbuffer_free(channels[channel].rbuf);
    channels[channel].rbuf = NULL;
    return 0;
}
EXPORT_SYSCALL(SYSCALL_IPC_CLOSE, sys_ipc_close);

int sys_ipc_send(int channel, void* data, int length)
{
    ERR_ON_NULL(data);
    IPC_VALID_CHANNEL(channel);

    return channels[channel].rbuf->ops->add(channels[channel].rbuf, data, length);
}
EXPORT_SYSCALL(SYSCALL_IPC_SEND, sys_ipc_send);

int sys_ipc_receive(int channel, void* data, int length)
{
    ERR_ON_NULL(data);
    IPC_VALID_CHANNEL(channel);

    return channels[channel].rbuf->ops->read(channels[channel].rbuf, data, length);
}
EXPORT_SYSCALL(SYSCALL_IPC_RECEIVE, sys_ipc_receive);
