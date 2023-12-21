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
#include <terminal.h>
#include <net/net.h>
#include <net/socket.h>
#include <net/ipv4.h>
#include <net/dns.h>

#include <scheduler.h>
#include <memory.h>
#include <util.h>
#include <serial.h>

#include <ksyms.h>
#include <kutils.h>
#include <kthreads.h>
#include <args.h>

static struct sock* client = NULL;

static int __net_terminal_writef(struct terminal* term, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char* buffer = kalloc(2000);
    int ret = csprintf(buffer, fmt, args);
    dbgprintf("Sending '%s' (%d bytes)\n", buffer, ret);
    
    if(client != NULL){
        kernel_send(client, buffer, ret, 0);
    }

    kfree(buffer);

    va_end(args);

    return 0;
}

void __kthread_entry tcp_server()
{
    struct terminal* term = terminal_create();
    if(term == NULL){
        dbgprintf("Unable to create terminal\n");
        kernel_exit();
    }

    struct terminal_ops ops = {
        .writef = __net_terminal_writef
    };
    term->ops->set(term, &ops);

    struct sock* socket = kernel_socket_create(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = INADDR_ANY;
    dest_addr.sin_port = htons(8080);
    dest_addr.sin_family = AF_INET;

    kernel_bind(socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));

    dbgprintf("TCP Server listening on port 8080\n");

    kernel_listen(socket, 5);

    while(1){
        struct sockaddr_in client_addr; 
        client = kernel_accept(socket, &client_addr, sizeof(client_addr));
        if (client == NULL){
            dbgprintf("Unable to accept connection: client is NULL\n");
            kernel_exit();
        }


        dbgprintf("Client connected from %i:%d\n", client_addr.sin_addr.s_addr, client_addr.sin_port);

        while(1){
            char buffer[100];

            int ret = kernel_recv(client, buffer, 2000, 0);
            if(ret <= 0){
                dbgprintf("Client disconnected\n");
                break;
            }

            buffer[ret] = 0;

            exec_cmd(buffer);

            // const char *http_response = 
            //     "HTTP/1.1 200 OK\r\n"
            //     "Content-Type: text/plain\r\n"
            //     "\r\n"
            //     "Hello, world!";

            // ret = kernel_send(client, http_response, strlen(http_response), 0);

        }
        

        kernel_sock_close(client);
    }
}
EXPORT_KTHREAD(tcp_server);

void __kthread_entry udp_server()
{
    struct sock* socket = kernel_socket_create(AF_INET, SOCK_DGRAM, 0);
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

    kernel_sock_close(socket);
}
EXPORT_KTHREAD(udp_server);

void __kthread_entry udptest()
{
    struct sock* socket = kernel_socket_create(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;

    dest_addr.sin_addr.s_addr = htonl(LOOPBACK_IP); 
    dest_addr.sin_port = htons(4242);
    dest_addr.sin_family = AF_INET;

    char* test = "Hello world!";
    int ret = kernel_sendto(socket, test, strlen(test), 0, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
    if(ret < 0){
        dbgprintf("Unable to send UDP packet\n");
    }

    kernel_sock_close(socket);
}
EXPORT_KTHREAD(udptest);

void __kthread_entry tcpd()
{
    int ret;
    struct sock* socket = kernel_socket_create(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest_addr;

    int ip = gethostname("tcpbin.com\0");
    if(ip == -1){
        dbgprintf("Unable to resolve tcpbin.com\n");
        kernel_exit();
    }

    dest_addr.sin_addr.s_addr = htonl(ip);
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