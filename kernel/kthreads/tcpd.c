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
#include <net/dns.h>

#include <scheduler.h>
#include <memory.h>
#include <util.h>
#include <serial.h>

#include <ksyms.h>
#include <kthreads.h>

void __kthread_entry local_udp_server()
{
    struct sock* socket = kernel_socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = INADDR_ANY;
    dest_addr.sin_port = htons(4242);
    dest_addr.sin_family = AF_INET;

    kernel_bind(socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

    dbgprintf("UDP Server listening on port 4242\n");

    char buffer[255];
    while(1){
        int ret = kernel_recv(socket, buffer, 255, 0);
        buffer[ret] = 0;

        dbgprintf(" Recieved '%s' (%d bytes)\n", buffer, ret);
    }
}
EXPORT_KTHREAD(local_udp_server);

void __kthread_entry udptest()
{
    struct sock* socket = kernel_socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(LOOPBACK_IP); 
    dest_addr.sin_port = htons(4242);
    dest_addr.sin_family = AF_INET;

    char* test = "Hello world!";
    int ret = kernel_sendto(socket, test, strlen(test), 0, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
    if(ret < 0){
        dbgprintf("Unable to send UDP packet\n");
    }
}
EXPORT_KTHREAD(udptest);

void __kthread_entry tcpd()
{
    int ret;
    struct sock* socket = kernel_socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest_addr;

    int ip = gethostname("tcpbin.com\0");
    if(ip == -1){
        dbgprintf("Unable to resolve tcpbin.com\n");
        kernel_exit();
    }

    dest_addr.sin_addr.s_addr = ip;
    dest_addr.sin_port = htons(4242);
    dest_addr.sin_family = AF_INET;

    kernel_connect(socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

    char* test = "Hello world!\n";
    ret = kernel_send(socket, test, strlen(test), 0);

    char reply[255];
    ret = kernel_recv(socket, reply, 255, 0);
    reply[ret] = 0;

    dbgprintf(" Reply '%s' (%d bytes)\n", reply, ret);

    kernel_sock_close(socket);
    kernel_exit();
}