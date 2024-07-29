#include "../../apps/utils/http/HttpParser.hpp"
#include "../../apps/utils/StringHelper.hpp"
#include "../../apps/utils/List.hpp"

#include <cstdlib>

const char* exmaple = "GET /index.html HTTP/1.1\r\n"
                      "Host: www.example.com\r\n"
                      "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3\r\n"
                      "Content-Type: text/html\r\n"
                      "Content-Length: 0\r\n"
                      "Connection: keep-alive\r\n"
                      "Date: Mon, 27 Jul 2009 12:28:53 GMT\r\n"
                      "Server: Apache/2.2.14 (Win32)\r\n"
                      "Set-Cookie: session-id=1234567890\r\n"
                      "\r\n";

class MyController : public HTTPEngine::Controller {
public:
    MyController() {}
    void init() {
        HTTPEngine::Route route("/index.html", GET, Function([this]() {
            printf("GET /index.html\n");
        }));
    }
private:

    String body;
};

// Controllers should get engine from constructor.
// I want to be able to add syntax like this: app.addService<MyController>(Singleton);


class MyEngine : public HTTPEngine {
public:
    MyEngine() {}
    void init();

    template <typename T>
    void addService(int lifetime) {
        size_t type_id = getTypeID<T>();
    }

private:
    String body;
};

void MyEngine::init() {
    MyController controller;

}

int main() {
    String str(exmaple);
    HTTPEngine parser;

    parser.parseRequest(str);
    return 0;
}   
