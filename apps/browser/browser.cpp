#include <lib/printf.h>
#include <libc.h>
#include <utils/http/HttpEngine.hpp>

#include "html.hpp"

static const char* default_host = "info.cern.ch";
static const char* default_path = "/";

int main() {
    printf("Starting browser...\n");
    http::Request req;
    http::Response res;
    printf("Fetching %s%s\n", default_host, default_path);

    req.setMethod(HTTP_GET);
    req.setPath(default_path);
    req.addHeader("User-Agent", "RetrOSBrowser/0.1");

    if (!http::HttpEngine::Send(default_host, 80, req, res)) {
        printf("Failed to fetch %s%s\n", default_host, default_path);
        return 1;
    }

    printf("Status: %s\n", http_errors[res.raw().status]);

    const char* body = res.raw().body ? res.raw().body : "";
    HTMLParser parser(body);
    int ret = parser.parse();
    if (ret != 0) {
        printf("HTML parse error: %s\n", HTMLParser::getHtmlError(ret));
        return 1;
    }

    parser.printTree();
    return 0;
}
