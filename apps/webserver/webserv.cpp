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
    web::FileRepository fileRepo;

    /* Simple static pages */
    webEngine.get("/home", [&fileRepo](const http::Request& req, http::Response& res) {
        (void)req;
        res.sendFile(fileRepo, "/web/index.htm");
    });

    webEngine.get("/about", [&fileRepo](const http::Request& req, http::Response& res) {
        (void)req;
        res.sendFile(fileRepo, "/web/about.htm");
    });

    webEngine.get("/status", [&fileRepo](const http::Request& req, http::Response& res) {
        (void)req;
        res.sendFile(fileRepo, "/web/status.htm");
    });

    webEngine.run();
    return 0;
}
