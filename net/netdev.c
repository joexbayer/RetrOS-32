/**
 * @file netdev.c
 * @author Joe Bayer (joexbayer)
 * @brief Network Device, handles all communication with network interface cards.
 * @version 0.1
 * @date 2022-06-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/netdev.h>
#include <terminal.h>

struct netdev current_netdev;
static uint8_t netdev_attached = 0;

void netdev_attach_driver(
    struct pci_device* driver, 
    int (*read)(char* buffer, uint32_t size), 
    int (*write)(char* buffer, uint32_t size),
    char* name,
    uint8_t* mac)
{
    current_netdev.driver = *driver;
    current_netdev.write = write;
    current_netdev.read = read;
    current_netdev.dropped = 0;
    current_netdev.received = 0;
    current_netdev.sent = 0;

    netdev_attached = 1;

    memcpy(current_netdev.mac, mac, 6);
    memcpy(current_netdev.name, name, strlen(name)+1);
}

int is_netdev_attached()
{
    return current_netdev.write != NULL;
}

int netdev_transmit(void* buffer, uint32_t size)
{
    current_netdev.sent++;
    int sent = current_netdev.write((char*) buffer, size);

    return sent;
}

int netdev_receive(void* buffer, uint32_t size)
{
    int read = current_netdev.read((char*) buffer, size);
    if(read <= 0)
        return -1;

    current_netdev.received++;

    return read;
}