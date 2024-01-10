/**
 * @file firewall.c
 * @author Joe Bayer (joexbayer)
 * @brief Firewall implementation.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdint.h>
#include <net/net.h>
#include <net/ethernet.h>
#include <net/arp.h>

#include <memory.h>

typedef enum __firewall_policy_t {
    FIREWALL_POLICY_ACCEPT,
    FIREWALL_POLICY_DROP,
    FIREWALL_POLICY_REJECT,
} firewall_policy_t;

struct net_firewall_rule {
    firewall_policy_t policy;
    
    uint32_t src_ip;
    uint32_t src_mask;

    uint32_t dst_ip;
    uint32_t dst_mask;

    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
};

struct net_firewall {
    struct net_firewall_rule* rules;
    int num_rules;
};

struct net_firewall* net_firewall_create()
{
    struct net_firewall* fw = create(struct net_firewall);
    if(fw == NULL) return NULL;

    fw->rules = NULL;
    fw->num_rules = 0;

    return fw;
}
