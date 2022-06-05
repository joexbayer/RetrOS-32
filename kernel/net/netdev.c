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

struct netdev current_netdev;

void netdev_attach_driver(
    struct pci_device* driver, 
    int (*read)(char* buffer, uint32_t size), 
    int (*write)(char* buffer, uint32_t size),
    char* name)
{
    current_netdev.driver = *driver;
    current_netdev.write = write;
    current_netdev.read = read;
    current_netdev.dropped = 0;
    current_netdev.received = 0;
    current_netdev.sent = 0;
    memcpy(current_netdev.name, name, strlen(name)+1);
}

int netdev_transmit(void* buffer, uint32_t size)
{
    current_netdev.sent++;
    int sent = current_netdev.write((char*) buffer, size);
    
    return sent;
}

int netdev_recieve(void* buffer, uint32_t size)
{
    int read = current_netdev.read((char*) buffer, size);
    if(read <= 0)
        return -1;

    current_netdev.received++;

    return read;
}