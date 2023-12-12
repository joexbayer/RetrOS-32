#include <net/interface.h>
#include <kutils.h>
#include <errors.h>
#include <memory.h>

static int __iface_send(struct net_interface* interface, void* buffer, uint32_t size);
static int __iface_recieve(struct net_interface* interface, void* buffer, uint32_t size);
static int __iface_assign(struct net_interface* interface, uint32_t ip);
static int __iface_attach(struct net_interface* interface, struct netdev* device);
static int __iface_detach(struct net_interface* interface);
static int __iface_set_gateway(struct net_interface* interface, uint32_t gateway);
static int __iface_set_netmask(struct net_interface* interface, uint32_t netmask);
static int __iface_configure(struct net_interface* interface, char* name);

static struct net_interface_ops default_iface_ops = {
    .send = __iface_send,
    .recieve = __iface_recieve,
    .assign = __iface_assign,
    .attach = __iface_attach,
    .detach = __iface_detach,
    .set_gateway = __iface_set_gateway,
    .set_netmask = __iface_set_netmask,
    .configure = __iface_configure
};

int net_register_netdev(char* name, struct netdev* device)
{
    /* validate net device */
    ERR_ON_NULL(device);
    ERR_ON_NULL(device->read);
    ERR_ON_NULL(device->write);

    struct net_interface* interface = net_interface_create();
    if(interface == NULL) {
        return -1;
    }

    interface->ops->attach(interface, device);
    interface->ops->assign(interface, 0);
    interface->ops->set_gateway(interface, 0);
    interface->ops->set_netmask(interface, 0);
    interface->ops->configure(interface, name);
    interface->state = NET_IFACE_UP;

    dbgprintf("Registering interface %s\n", interface->name);

    return net_register_interface(interface);
}

struct net_interface* net_interface_create()
{
    struct net_interface* interface = (struct net_interface*)kalloc(sizeof(struct net_interface));
    if(interface == NULL) {
        return NULL;
    }

    interface->device = NULL;
    interface->ip = 0;
    interface->netmask = 0;
    interface->gateway = 0;
    interface->ops = &default_iface_ops;

    return interface;
}

static int __iface_send(struct net_interface* interface, void* buffer, uint32_t size)
{
    if(interface->device == NULL) {
        return -1;
    }

    return interface->device->write(buffer, size);
}

static int __iface_recieve(struct net_interface* interface, void* buffer, uint32_t size)
{
    if(interface->device == NULL) {
        return -1;
    }

    return interface->device->read(buffer, size);
}

static int __iface_assign(struct net_interface* interface, uint32_t ip)
{
    interface->ip = ip;
    return 0;
}

static int __iface_attach(struct net_interface* interface, struct netdev* device)
{
    interface->device = device;
    memcpy(interface->mac, device->mac, ETHER_HDR_LENGTH);
    return 0;
}

static int __iface_detach(struct net_interface* interface)
{
    interface->device = NULL;
    return 0;
}

static int __iface_set_gateway(struct net_interface* interface, uint32_t gateway)
{
    interface->gateway = gateway;
    return 0;
}

static int __iface_set_netmask(struct net_interface* interface, uint32_t netmask)
{
    interface->netmask = netmask;
    return 0;
}

static int __iface_configure(struct net_interface* interface, char* name)
{
    ERR_ON_NULL(name);    

    memcpy(interface->name, name, 16);
    return 0;
}

int net_interface_destroy(struct net_interface* interface)
{
    /* deal with device? */
    kfree(interface);
    return 0;
}




