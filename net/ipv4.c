/**
 * @file ipv4.c
 * @author Joe Bayer (joexbayer)
 * @brief Implementation of the IPv4 protocol.
 * @version 0.1
 * @date 2022-06-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/ipv4.h>
#include <net/utils.h>
#include <terminal.h>
#include <util.h>
#include <net/arp.h>
#include <net/ethernet.h>

/**
 * @brief Helper function to send IP packet, attaching ethernet header
 * and setting skb to be sent.
 * 
 * @param ihdr ip header for packet, needs to be configured before this function.
 * @param skb socket buffer to modify.
 * @param dip Destination IP address.
 * 
 * @returns int
 */
int __ip_send(struct ip_header* ihdr, struct sk_buff* skb, uint32_t dip)
{
    if(ihdr->version == 0)
        return 0;

    skb->proto = IP;

	twriteln("Creating Ethernet header.");
	int ret = ethernet_add_header(skb, dip);
	if(ret <= 0){
		twriteln("Error adding ethernet header");
		return 0;
	}

    /* Add IP header to packet */
    memcpy(skb->data, ihdr, sizeof(struct ip_header));
    skb->len += ihdr->ihl * 4;
    skb->data += ihdr->ihl * 4;

    return 1;
}

/**
 * @brief Creates and attaches IP header to SKB
 * 
 * @param skb skb to modify
 * @param ip destination IP
 * @param proto TCP / UDP
 * @param length length of message.
 * @return int 
 */
int ip_add_header(struct sk_buff* skb, uint32_t ip, uint8_t proto, uint32_t length)
{
    struct ip_header hdr;
    IP_HEADER_CREATE(hdr, proto, length);

    /* Set IPs */
    hdr.saddr = BROADCAST_IP; /* TODO */
    hdr.daddr = ip;

    IP_HTONL(&hdr);

    hdr.csum = 0;
    hdr.csum = checksum(&hdr, hdr.ihl * 4, 0);

    return __ip_send(&hdr, skb, ip);
}

/**
 * @brief parses a packets IP header.
 * 
 * @param skb skb packet to parse
 * @return int 
 */
int ip_parse(struct sk_buff* skb)
{
    struct ip_header* hdr = (struct ip_header* ) skb->data;
    skb->hdr.ip = hdr;

    /**
     * @brief Calculate checksum of IP packet
     * and validate that it is correct.
     */
    uint16_t csum = checksum(hdr, hdr->ihl * 4, 0);
    if(0 != csum){
        twriteln("Checksum failed (IPv4)");
        return 0;
    }

    IP_NTOHL(hdr);
    skb->data = skb->data+(skb->hdr.ip->ihl*4);

    if(BROADCAST_IP != ntohl(skb->hdr.ip->daddr)){
        // check if its current IP
        return 0; /* Currently only accept broadcast packets. */
    }

    char mac[6];
    int arp = arp_find_entry(hdr->saddr, (uint8_t*)&mac);
    if(arp == 0)
    {
        struct arp_content content;
        content.sip = hdr->saddr;

        struct ethernet_header* ehdr= (struct ethernet_header*) skb->head;
        memcpy(&content.smac, ehdr->smac, 6);

        arp_add_entry(&content);
    }


    return 1;
}

void print_ip(uint32_t ip)
{
    unsigned char bytes[4];
    bytes[0] = (ip >> 24) & 0xFF;
    bytes[1] = (ip >> 16) & 0xFF;
    bytes[2] = (ip >> 8) & 0xFF;
    bytes[3] = ip & 0xFF;   
    twritef("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]); 
}

/*  Function from: https://www.lemoda.net/c/ip-to-integer/ */
uint32_t ip_to_int (const char * ip)
{
    unsigned v = 0;
    int i;
    const char * start;

    start = ip;
    for (i = 0; i < 4; i++) {
        char c;
        int n = 0;
        while (1) {
            c = * start;
            start++;
            if (c >= '0' && c <= '9') {
                n *= 10;
                n += c - '0';
            } else if ((i < 3 && c == '.') || i == 3) {
                break;
            } else {
                return 0;
            }
        }
        if (n >= 256) {
            return 0;
        }
        v *= 256;
        v += n;
    }
    return v;
}