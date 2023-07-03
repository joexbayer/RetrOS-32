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