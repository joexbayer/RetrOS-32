#ifndef __INTERFACE_H
#define __INTERFACE_H

#include <net/ethernet.h>
#include <net/ipv4.h>
#include <stdint.h>
#include <net/netdev.h>

typedef enum {
    NET_IFACE_UP,
    NET_IFACE_DOWN
} net_iface_state_t;

struct net_interface;

struct net_interface_ops {
    int (*send)(struct net_interface* interface, void* buffer, uint32_t size);
    int (*recieve)(struct net_interface* interface, void* buffer, uint32_t size);
    int (*assign)(struct net_interface* interface, uint32_t ip);
    int (*attach)(struct net_interface* interface, struct netdev* device);
    int (*detach)(struct net_interface* interface);
    int (*set_gateway)(struct net_interface* interface, uint32_t gateway);
    int (*set_netmask)(struct net_interface* interface, uint32_t netmask);
    int (*configure)(struct net_interface* interface, char* name);

    int (*destroy)(struct net_interface* interface);
};

struct net_interface {
    struct net_interface_ops* ops;
    uint8_t mac[ETHER_HDR_LENGTH];
    net_iface_state_t state;
    struct netdev* device;
    uint32_t netmask;
    uint32_t gateway;
    char name[16];
    uint32_t ip;
};

struct net_interface* net_interface_create();
int net_register_netdev(char* name, struct netdev* device);

#endif // !__INTERFACE_H