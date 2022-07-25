#ifndef DNS_H
#define DNS_H

#include <stdint.h>
#include <net/utils.h>

#define DNS_T_A 1           // ipv4 address
#define DNS_T_NS 2          // nameserver
#define DNS_T_CNAME 5       // canonical name
#define DNS_T_SOA 6         // start of authority zone
#define DNS_T_PTR 12        // domain name pointer
#define DNS_T_MX 15         // mail server

#define DNS_CACHE_ENTRIES 10

struct dns_header
{
    uint16_t id;
 
    uint8_t rd :1;          // recursion desired
    uint8_t tc :1;          // truncated message
    uint8_t aa :1;          // authoritive answer
    uint8_t opcode :4;      // option code
    uint8_t qr :1;          // query/response flag
 
    uint8_t rcode :4;       // response code
    uint8_t cd :1;          // checking disabled
    uint8_t ad :1;          // authenticated data
    uint8_t z :1;           // reserved
    uint8_t ra :1;          // recursion available
 
    uint16_t q_count;       // number of question entries
    uint16_t ans_count;     // number of answer entries
    uint16_t auth_count;    // number of authority entries
    uint16_t add_count;     // number of resource entries
} __attribute__((__packed__));

struct dns_question
{
    uint16_t qtype;
    uint16_t qclass;
};

struct dns_answer
{
    uint16_t name;
    uint16_t type;
    uint16_t _class;
    uint16_t ttl;
    uint16_t ttl1;
    uint16_t data_len;
};

//Structure of a Query
struct dns_query
{
    uint8_t *name;
    struct dns_question question;
};

struct dns_cache
{
    char name[10];
    uint32_t ip;
};

#define DNS_REQUEST(dns) \
    (dns)->id = 0; \
    (dns)->qr = 0; \
    (dns)->opcode = 0; \
    (dns)->aa = 0; \
    (dns)->tc = 0; \
    (dns)->rd = 1; \
    (dns)->ra = 0; \
    (dns)->z = 0; \
    (dns)->ad = 0; \
    (dns)->cd = 0; \
    (dns)->rcode = 0; \
    (dns)->q_count = htons(1); \
    (dns)->ans_count = 0; \
    (dns)->auth_count = 0; \
    (dns)->add_count = 0; \

void init_dns();
int gethostname(char* hostname);


#endif /* DNS_H */
