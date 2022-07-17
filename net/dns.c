/**
 * @file dns.c
 * @author your name (you@domain.com)
 * @brief Domain Name Server implementation.
 * @version 0.1
 * @date 2022-07-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <net/dns.h>
#include <net/dhcpd.h>

static struct dns_cache __dns_cache[DNS_CACHE_ENTRIES];

void init_dns();
int get_hostname(char* hostname);

void init_dns()
{
    /* Set DNS cache to be "empty". */
    for (int i = 0; i < DNS_CACHE_ENTRIES; i++)
    {
        __dns_cache[i].ip = 0;
    }
}