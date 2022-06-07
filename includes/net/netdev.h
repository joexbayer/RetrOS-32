#ifndef NETDEV_H
#define NETDEV_H

#include <util.h>
#include <pci.h>

#define MAX_NETDEV_NAME_SIZE 10

extern struct netdev current_netdev;  

/**
 * @brief Main struct that keeps track of the network, especially its stats and read / write functions.
 * 
 */
struct netdev {
    char name[MAX_NETDEV_NAME_SIZE];

    uint32_t sent;
    uint32_t received;
    uint32_t dropped;

    uint16_t mac_address[6];

    struct pci_device driver;

    int32_t (*read)(char* buffer, uint32_t size);
    int32_t (*write)(char* buffer, uint32_t size);
};

/**
 * @brief Attaches adriver to the current network device. Also attaches functions for reading and writing.
 * This makes changing netdev very easy.
 * 
 * @param driver pci driver device.
 * @param read function that will read from the network card.
 * @param write function that will write to the network card.
 * @param name Name of the current network card.
 */
void netdev_attach_driver(
    struct pci_device* driver, 
    int (*read)(char* buffer, uint32_t size), 
    int (*write)(char* buffer, uint32_t size),
    char* name
);

void netdev_set_macaddr(uint16_t* mac);
void netdev_print_status();

/* TODO: add to own headerfile. */
void networking_print_status();
void net_new_packet();

int netdev_recieve(void* buffer, uint32_t size);
int netdev_transmit(void* buffer, uint32_t size);


#endif /* NETDEV_H */
