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

class MyService : public HTTPEngine::Service {
public:
    MyService(HTTPEngine::ServiceContainer* container) {}
    void init() {
        printf("MyService init\n");
    }

    void speak() {
        printf("MyService speak\n");
    }
};

class MyController : public HTTPEngine::Controller {
public:
    MyController(HTTPEngine::ServiceContainer* container) : HTTPEngine::Controller(container) {
        printf("MyController constructor\n");
    }

    ~MyController() {
        printf("MyController destructor\n");
    }
    

    void init() {

        MyService* service = container->getService<MyService>();
        if(service == nullptr) {
            printf("Service is null\n");
        }

        HTTPEngine::Route route("/index.html", GET, Function([this]() {
            response.setBody("Hello, World!");
            response.addHeader(HTTPHeader(CONTENT_TYPE, "text/html"));
            printf("Route called\n");
        }));
        addRoute(route);

        printf("MyController init\n");

        if(service != nullptr) {
            service->speak();
        }

        route.call();

        printf("Response %s\n", response.getBody().getData());

        delete service;
    }
private:
};

class MyEngine : public HTTPEngine {
public:
    MyEngine() {}
    void init();

private:
    String body;
};

void MyEngine::init() {
    addService<MyController>(Singleton);
    addService<MyService>(Singleton);
}

int main() {
    String str(exmaple);
    MyEngine parser;
    parser.init();

    parser.parseRequest(str);
    printf("Request parsed\n");
    return 0;
}   
