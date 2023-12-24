#ifndef B37DA3E3_2C55_4777_BB81_B23801925C4B
#define B37DA3E3_2C55_4777_BB81_B23801925C4B

#include <stdint.h>
#include <net/interface.h>
#include <net/netdev.h>
#include <net/net.h>
#include <net/packet.h>
#include <net/skb.h>
#include <net/ethernet.h>
#include <pcb.h>

enum NETD_STATES {
    NETD_UNINITIALIZED,
    NETD_STARTED
};

struct networkmanager;
struct network_manager_ops {
    int (*start)(struct networkmanager*);
    int (*stop)(struct networkmanager*);
    int (*restart)(struct networkmanager*);

    void (*get_info)(struct net_info* info);
    void (*send_skb)(struct networkmanager* nm, struct sk_buff* skb);
};

struct networkmanager {
    int state;

    uint16_t packets;
    struct skb_queue* skb_tx_queue;
    struct skb_queue* skb_rx_queue;

    struct net_info stats;

    struct network_manager_ops* ops;

    struct net_interface* ifs[4];
    uint8_t if_count;

    struct pcb* instance;

    struct kref ref;

};


#endif /* B37DA3E3_2C55_4777_BB81_B23801925C4B */
