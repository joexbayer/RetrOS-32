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
#include <screen.h>

struct netdev current_netdev;

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

    memcpy(current_netdev.mac, mac, 6);
    memcpy(current_netdev.name, name, strlen(name)+1);
}

void netdev_print_status()
{
    scrwrite(51, 13, "Card: ", VGA_COLOR_GREEN);
    scrprintf(51+strlen("Card: "), 13, "%s", current_netdev.name);
    scrprintf(51, 14, "PCI BAR0: 0x%x", current_netdev.driver.base);
    scrprintf(51, 15, "Transmit: %d", current_netdev.sent);
    scrprintf(51, 16, "Recieved: %d (%d dropped)", current_netdev.received, current_netdev.dropped);

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