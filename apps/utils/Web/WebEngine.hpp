#ifndef WEBENGINE_HPP
#define WEBENGINE_HPP

#include <utils/StdLib.hpp>
#include <stdint.h>
#include <lib/net.h>
#include <args.h>
#include <lib/printf.h>
#include <utils/Function.hpp>
#include <lib/http.h>
#include <utils/http/HttpEngine.hpp>

namespace web {

    using RouteHandler = Function<void(const http::Request&, http::Response&)>;

    enum Method {
        GET,
        POST
    };

    class Router {
    public:
        Router() {}
        ~Router() {}

        void addRoute(const char* path, Method method, RouteHandler handler) {
            if (route_count < 64) {
                routes[route_count].path = path;
                routes[route_count].method = method;
                /* move-assign to avoid deleted copy assignment */
                routes[route_count].handler = RouteHandler(handler);
                route_count++;
            }
        }

        bool handleRequest(const http::Request& req, http::Response& res) {
            for (size_t i = 0; i < route_count; i++) {
                if (strcmp(req.path(), routes[i].path) == 0 && methodsMatch(routes[i].method, req.method())) {
                    routes[i].handler(req, res);
                    return true;
                }
            }
            return false;
        }
        
    private:
        struct Route {
            const char* path;
            web::Method method;
            RouteHandler handler;
        };

        static bool methodsMatch(web::Method routeMethod, http_method_t reqMethod) {
            switch (routeMethod) {
                case web::GET:  return reqMethod == HTTP_GET;
                case web::POST: return reqMethod == HTTP_POST;
                default:        return false;
            }
        }

        struct Route routes[64];
        size_t route_count = 0;
    };
};

class WebEngine {
public:
    enum LogLevel {
        ERROR,
        WARN,
        INFO,
        DEBUG
    };

    WebEngine(uint16_t port = 80, int backlog = 16, LogLevel logLevel = INFO) : httpEngine(), logLevel(logLevel) {
        server = new TcpServer(port, backlog);
    }
    ~WebEngine() {
        delete server;
    }

    int get(const char* path, web::RouteHandler handler) {
        router.addRoute(path, web::Method::GET, handler);
        return 0;
    }

    int post(const char* path, web::RouteHandler handler) {
        router.addRoute(path, web::Method::POST, handler);
        return 0;
    }

    void run() {
        Log(INFO, "[WEBENGINE] Starting web server...");

        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        char* recvBuffer = new char[HTTP_RESPONSE_SIZE]; 
        char* sendBuffer = new char[HTTP_RESPONSE_SIZE];

        while (running) {
            addr_len = sizeof(client_addr);
            int client = server->accept((struct sockaddr*)&client_addr, &addr_len);
            if (client < 0) {
                Log(INFO, "[WEBENGINE] Failed to accept client connection");
                continue;
            }

            int ret = recv(client, recvBuffer,  HTTP_RESPONSE_SIZE - 1, 0);
            if (ret <= 0) {
                Log(WARN, "[WEBENGINE] recv error or connection closed read %d", ret);
                close(client);
                continue;
            }

            recvBuffer[ret] = 0;

            http::Request req;
            http::Response res;

            if (!http::HttpEngine::Parse(recvBuffer, req)) {
                res.setStatus(HTTP_400_BAD_REQUEST);
                res.setBody("Bad Request");
                printf("[WEBENGINE] Failed to parse HTTP request\n");
            } else {
                bool handled = router.handleRequest(req, res);
                if (!handled) {
                    res.setStatus(HTTP_404_NOT_FOUND);
                    res.setBody("Not Found");
                    printf("[WEBENGINE] Request not handled, returning 404\n");
                }
            }

            int response_len = http::HttpEngine::BuildResponse(res, sendBuffer, HTTP_RESPONSE_SIZE);
            if (response_len > 0) {
                printf("[WEBENGINE] Sending response:\n%s\n", sendBuffer);
                send(client, sendBuffer, response_len, 0);
            }
            close(client);

            memset(recvBuffer, 0, HTTP_RESPONSE_SIZE);
            memset(sendBuffer, 0, HTTP_RESPONSE_SIZE);
        }
        delete[] recvBuffer;
        delete[] sendBuffer;
        Log(INFO, "[WEBENGINE] Web server stopped.");
    }

private:
    TcpServer* server;

    void Log(LogLevel level, const char* message, ... ) {
        if (level <= logLevel) {
            va_list args;
            va_start(args, message);
            printf(message, args);
            va_end(args);
        }
        printf("\n");
    }

    http::HttpEngine httpEngine;
    web::Router router;
    LogLevel logLevel;

    bool running = true;
};

#endif // WEBENGINE_HPP
