/**
 * @file tcpd.c
 * @author Joe Bayer (joexbayer)
 * @brief Testing TCP 
 * @version 0.1
 * @date 2023-04-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <net/net.h>
#include <net/socket.h>
#include <net/ipv4.h>

#include <scheduler.h>
#include <memory.h>
#include <util.h>
#include <serial.h>

void tcpd()
{
    int ret;
    /* Create and bind DHCP socket to DHCP_SOURCE_PORT and INADDR_ANY. */
    struct sock* tcp_socket = kernel_socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(ip_to_int("45.79.112.203"));
    dest_addr.sin_port = htons(4242);
    dest_addr.sin_family = AF_INET;

    kernel_connect(tcp_socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

    char* test = "Hello world!\n";
    dbgprintf(" Sending '%s'\n", test);
    ret = kernel_send(tcp_socket, test, strlen(test), 0);

    char reply[255];
    ret = kernel_recv(tcp_socket, reply, 255, 0);
    reply[ret] = 0;

    dbgprintf(" Reply '%s' (%d bytes)\n", reply, ret);

    while(1);
}