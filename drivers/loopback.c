/**
 * @file loopback.c
 * @author Joe Bayer (joexbayer)
 * @brief Loopback interface for internal networking.
 * @version 0.1
 * @date 2023-12-12
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <net/interface.h>
#include <work.h>
#include <net/netdev.h>
#include <net/net.h>
#include <memory.h>
#include <math.h>

static struct queue {
    struct queue_entry {
        void* data;
        int size;
    } entries[32];
    int head;
    int tail;
    unsigned int size;
} loopback_queue = {0};

static int iface_loopback_read(char* buffer, uint32_t size);
static int iface_loopback_write(char* buffer, uint32_t size);

static struct netdev loopback_device = {
    .name = "loopback",
    .read = iface_loopback_read,
    .write = iface_loopback_write,
    .sent = 0,
    .received = 0,
    .dropped = 0,
    .mac = {0x69, 0x00, 0x00, 0x00, 0x00, 0x00}
};

/* emulating a interrupt to deliver packet */
static int iface_loopback_interrupt(void* _)
{   
    net_incoming_packet(&loopback_device);
    return 0;
}

static int iface_loopback_read(char* buffer, uint32_t size)
{
    if(loopback_queue.size == 0)
        return -1;

    struct queue_entry* entry = &loopback_queue.entries[loopback_queue.head];
    int read = MIN(entry->size, size);
    memcpy(buffer, entry->data, MIN(entry->size, read));

    kfree(entry->data);
    entry->data = NULL;
    entry->size = 0;

    loopback_queue.head = (loopback_queue.head + 1) % 32;

    return read;
}

static int iface_loopback_write(char* buffer, uint32_t size)
{
    if(buffer == NULL)
        return -1;

    /* check if there is space */
    if(loopback_queue.size == 32)
        return -2;
    
    if(loopback_queue.entries[loopback_queue.tail].size != 0)
        return -3;

    char* data = (char*) kalloc(size);
    memcpy(data, buffer, size);

    loopback_queue.entries[loopback_queue.tail].data = data;
    loopback_queue.entries[loopback_queue.tail].size = size;

    loopback_queue.tail = (loopback_queue.tail + 1) % 32;
    loopback_queue.size++;

    work_queue_add(&iface_loopback_interrupt, NULL, NULL);

    return 0;
}

int net_init_loopback()
{
    return net_register_netdev("lo0", &loopback_device);
}

