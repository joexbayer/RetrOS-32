#include <utils/StdLib.hpp>
#include <lib/graphics.h>
#include <colors.h>
#include <args.h>
#include <libc.h>
#include <lib/syscall.h>
#include <lib/net.h>
#include <lib/printf.h>
#include <lib/http.h>
#include <utils/Function.hpp>
#include <utils/web/WebEngine.hpp>

int main()
{
    WebEngine webEngine(80, 16);

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
        "<li>Web Server: Port 80</li>\n"
        "<li>Status: Running</li>\n"
        "</ul>\n"
        "<hr>\n"
        "<p><i>Powered by RetrOS-32</i></p>\n"
        "</body>\n"
        "</html>\n";

    webEngine.get("/", [=](const http::Request& req, http::Response& res) {
        (void)req;
        printf("[WEBENGINE] Handled GET / request\n");
        
        res.setStatus(HTTP_200_OK);
        res.addHeader("Content-Type", "text/html");
        res.setBody(body);

    });

    webEngine.run();
    return 0;
}
