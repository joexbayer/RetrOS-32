/**
 * @file dns.c
 * @author Joe Bayer (joexbayer)
 * @brief Domain Name System implementation.
 * @version 0.1
 * @date 2022-07-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <net/dns.h>
#include <net/dhcp.h>
#include <net/socket.h>
#include <net/net.h>
#include <sync.h>
#include <serial.h>
#include <syscalls.h>
#include <syscall_helper.h>

#include <libc.h>

#define DNS_PORT 53

static struct dns_cache __dns_cache[DNS_CACHE_ENTRIES];
static mutex_t __dns_mutex;

void net_init_dns();
int gethostname(char* hostname);

void net_init_dns()
{
    /* Set DNS cache to be "empty". */
    for (int i = 0; i < DNS_CACHE_ENTRIES; i++){
        __dns_cache[i].ip = 0;
    }

    mutex_init(&__dns_mutex);
}

static void __dns_name_compression(uint8_t* request, char* host) 
{
    int lock = 0;
    host[strlen(host)] = '.';

    for(int i = 0 ; i < strlen(host); i++) {
        if(host[i]=='.') {
            *request++ = i-lock;
            for(;lock<i;lock++) {
                *request++=host[lock];
            }
            lock++; //or lock=i+1;
        }
    }
    *request++ = '\0';
}

static void __dns_add_cache(char* hostname, uint32_t ip)
{
    for (int i = 0; i < DNS_CACHE_ENTRIES; i++){
        if(__dns_cache[i].ip == 0){
            memcpy(__dns_cache[i].name, hostname, strlen(hostname));
            __dns_cache[i].ip = ip;
            return;
        }
    }
}

/* returns -1 on error */
int gethostname(char* hostname)
{

    if(dhcp_get_state() == DHCP_STOPPED){
        dbgprintf("[DNS] Unable to resolve hostname. No IP.");
        return -1;
    }

    /* Check for cache first. */
    for (int i = 0; i < DNS_CACHE_ENTRIES; i++)
        if(memcmp((uint8_t*) &__dns_cache[i].name,(uint8_t*) hostname, strlen(hostname)) == 0){
            dbgprintf("[DNS] (%s at %i) (cache)\n", hostname, __dns_cache[i].ip);
            return __dns_cache[i].ip;
        }

    dbgprintf("[DNS] query for (%s)\n", hostname);
    
    char hostname_save[40];
    memcpy(hostname_save, hostname, strlen(hostname)+1);
    //acquire(&__dns_mutex);

    struct sock* __dns_socket = kernel_socket_create(AF_INET, SOCK_DGRAM, 0);

    uint8_t buf[2048]; /* Can be replaced with alloc. */
    struct dns_header* request;
    struct dns_question* request_question;

    request = (struct dns_header*) &buf;
    DNS_REQUEST(request);

    /* Move pointer past header */
    uint8_t* question =(uint8_t*)&buf[sizeof(struct dns_header)];
    __dns_name_compression(question, hostname);
    request_question =(struct dns_question*) &buf[sizeof(struct dns_header) + (strlen((const char*)question) + 1)];
 
    request_question->qtype = htons(DNS_T_A); //type of the query , A , MX , CNAME , NS etc
    request_question->qclass = htons(1);

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(DNS_PORT);
    dest.sin_addr.s_addr = htonl(dhcp_get_dns());

    int question_size = sizeof(struct dns_header) + (strlen((const char*)question)+1) + sizeof(struct dns_question);
 
    kernel_sendto(__dns_socket, (char*)buf, question_size, 0, (struct sockaddr*)&dest, sizeof(dest));
    int ret = kernel_recv(__dns_socket, buf, 2048, 0);
    if(ret <= 0){
        dbgprintf("Failed DNS lookup\n");
        kernel_sock_close(__dns_socket);
        return -1;
    }
    
    struct dns_answer* answer;
    int last_length = 0;

    dbgprintf("[DNS] %i bytes received\n", ret);

    struct dns_header* dns = (struct dns_header*) &buf;
    for (int i = 0; i < dns->ans_count; i++){
        int next = (sizeof(struct dns_answer)*i) + last_length;
        answer = (struct dns_answer*) &buf[question_size+next];
        last_length = ntohs(answer->data_len);
        
        if(answer->type != ntohs(DNS_T_A))
            continue;

        uint32_t result;
        switch (ntohs(answer->data_len)){
        case 4:
            result = *((uint32_t*) &buf[question_size+12+next]);
            break;
        
        default:
            result = 0;
            break;
        }

        dbgprintf("[DNS]: %x name\n", ntohs(answer->name));
        dbgprintf("[DNS]: %x type\n",  ntohs(answer->type));
        dbgprintf("[DNS]: %x _class\n",ntohs(answer->_class));
        dbgprintf("[DNS]: %x ttl\n", ntohl(answer->ttl));
        dbgprintf("[DNS]: %x len\n", ntohs(answer->data_len));

        if(result <= 0){
            dbgprintf("[DNS] Unable to resolve hostname.");
            //release(&__dns_mutex);
            kernel_sock_close(__dns_socket);
            return -1;
        }

        dbgprintf("[DNS] (%s at %i)\n", hostname, result);

        __dns_add_cache(hostname_save, result);

        kernel_sock_close(__dns_socket);

        //release(&__dns_mutex);

        dbgprintf("[DNS] (%s at %i) (new)\n", hostname, result);

        return result;
    }
    return 0;
}
EXPORT_SYSCALL(SYSCALL_NET_DNS_LOOKUP, gethostname);