#include <lib/graphics.h>
#include <colors.h>
#include <args.h>
#include <libc.h>
#include <lib/syscall.h>
#include <lib/net.h>
#include <lib/printf.h>

int main()
{
    TcpServer server(80, 16);

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    const char* body = 
        "<html>\n"
        "<head><title>RetrOS-32</title></head>\n"
        "<body>\n"
        "<h1>Welcome to RetrOS-32</h1>\n"
        "<hr>\n"
        "<p>This is a simple web server running on RetrOS-32.</p>\n"
        "<p><b>System Information:</b></p>\n"
        "<ul>\n"
        "<li>Operating System: RetrOS-32</li>\n"
        "<li>Web Server: Port 8080</li>\n"
        "<li>Status: Running</li>\n"
        "</ul>\n"
        "<hr>\n"
        "<p><i>Powered by RetrOS-32</i></p>\n"
        "</body>\n"
        "</html>\n"
        "\r\n";
    char response[512];
    
    int response_len = sprintf(response,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        (int)strlen(body), body);
    char buffer[4048] = {0};
    
    while(1){
        printf("[WEBSERV] Waiting for client connection...\n");
        addr_len = sizeof(client_addr);
        int client = server.accept((struct sockaddr*)&client_addr, &addr_len);
        if (client < 0) {
            printf("[WEBSERV] Failed to accept client connection\n");
            continue;
        }

        int ret = recv(client, buffer, sizeof(buffer) - 1, 0);
        //printf("recv returned %d\n", ret);
        if (ret <= 0) {
            printf("[WEBSERV] recv error or connection closed read %d\n", ret);
            close(client);
            continue;
        }

        buffer[ret] = 0;
        //printf("Received: %s\n", buffer);
        printf("[WEBSERV] Serving response to client %d %d\n", client, response_len);
        send(client, response, response_len, 0);
        printf("[WEBSERV] Response sent to client %d\n", client);
        close(client);
        printf("[WEBSERV] Connection with client %d closed\n", client);
    }
    return 0;

    // struct gfx_event e;
    // while (1){
    //     gfx_get_event(&e, GFX_EVENT_BLOCKING); /* alt: GFX_EVENT_NONBLOCKING */
    //     switch (e.event)
    //     {
    //     case GFX_EVENT_RESOLUTION:
    //         /* update screensize */
    //         break;
    //     case GFX_EVENT_EXIT:
    //         /* exit */
    //         return 0;
    //     case GFX_EVENT_KEYBOARD:
    //         /* keyboard event in e.data */
    //         break;
    //     case GFX_EVENT_MOUSE:
    //         /* mouse event in e.data and e.data2 */
    //         break;
    //     }

    // }

    return 0;
}
