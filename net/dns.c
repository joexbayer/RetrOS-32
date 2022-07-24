/**
 * @file dns.c
 * @author Joe Bayer (joexbayer)
 * @brief Domain Name Server implementation.
 * @version 0.1
 * @date 2022-07-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <net/dns.h>
#include <net/dhcpd.h>
#include <net/socket.h>
#include <sync.h>

#include <util.h>

static struct dns_cache __dns_cache[DNS_CACHE_ENTRIES];
static socket_t __dns_socket;
static mutex_t __dns_mutex;

void init_dns();
int gethostname(char* hostname);

void init_dns()
{
    /* Set DNS cache to be "empty". */
    for (int i = 0; i < DNS_CACHE_ENTRIES; i++)
    {
        __dns_cache[i].ip = 0;
    }

    __dns_socket = socket(AF_INET, SOCK_DGRAM, 0);
    mutex_init(&__dns_mutex);

}

static void __dns_name_compresion(uint8_t* request, char* host) 
{
    int lock = 0;
    host[strlen(host)+1] = '.';

    for(int i = 0 ; i < strlen(host); i++) 
    {
        if(host[i]=='.') 
        {
            *request++ = i-lock;
            for(;lock<i;lock++) 
            {
                *request++=host[lock];
            }
            lock++; //or lock=i+1;
        }
    }
    *request++ = '\0';
}


int gethostname(char* hostname)
{
    /* Check for cache first. */
    for (int i = 0; i < DNS_CACHE_ENTRIES; i++)
        if(memcmp((uint8_t*) &__dns_cache[i].name,(uint8_t*) hostname, strlen(hostname)))
            return __dns_cache[i].ip;
    
    acquire(&__dns_mutex);

    uint8_t buf[65536]; /* Can be replaced with alloc. */
    struct dns_header* request;
    struct dns_question* request_question;

    request = (struct dns_header*) &buf;
    DNS_REQUEST(request);

    /* Move pointer past header */
    uint8_t* question =(uint8_t*)&buf[sizeof(struct dns_header)];
    __dns_name_compresion(question, hostname);

    request_question =(struct dns_question*) &buf[sizeof(struct dns_header) + (strlen((const char*)question) + 1)];
 
    request_question->qtype = htons(DNS_T_A); //type of the query , A , MX , CNAME , NS etc
    request_question->qclass = htons(1);

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = htonl(dhcp_get_dns()); //dns servers
 
    sendto(__dns_socket, (char*)buf, sizeof(struct dns_header) + (strlen((const char*)question)+1) + sizeof(struct dns_question), 0, (struct sockaddr*)&dest, sizeof(dest));

    release(&__dns_mutex);

}