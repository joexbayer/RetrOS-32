#include <lib/graphics.h>
#include <colors.h>
#include <args.h>
#include <libc.h>
#include <lib/syscall.h>
#include <lib/net.h>
#include <lib/printf.h>

int main()
{
    TcpServer server(80);

    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nHello, World!";
    char buffer[4048] = {0};
    while(1){
        addr_len = sizeof(client_addr);
        int client = server.accept((struct sockaddr*)&client_addr, &addr_len);
        if (client < 0) {
            continue;
        }

        int ret = recv(client, buffer, sizeof(buffer) - 1, 0);
        printf("recv returned %d\n", ret);
        if (ret <= 0) {
            close(client);
            continue;
        }

        buffer[ret] = 0;
        printf("Received: %s\n", buffer);
        send(client, response, strlen(response)-1, 0);
        close(client);
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
