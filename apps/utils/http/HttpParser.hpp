#ifndef A708D386_C059_4ABA_94C8_F1D39D13EB36
#define A708D386_C059_4ABA_94C8_F1D39D13EB36

#include <StringHelper.hpp>
#include <Function.hpp>
#include <List.hpp>
#include <stdlib.h>
#include <cstdio>

enum HTTPMethod {
    GET,
    POST,
    PUT,
    DELETE,
    HEAD,
    OPTIONS,
    TRACE,
    CONNECT,
    PATCH
};

enum HTTPVersion {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0
};

enum HTTPStatusCode {
    OK                      = 200,
    CREATED                 = 201,
    ACCEPTED                = 202,
    NO_CONTENT              = 204,
    MOVED_PERMANENTLY       = 301,
    FOUND                   = 302,
    SEE_OTHER               = 303,
    NOT_MODIFIED            = 304,
    BAD_REQUEST             = 400,
    UNAUTHORIZED            = 401,
    FORBIDDEN               = 403,
    NOT_FOUND               = 404,
    METHOD_NOT_ALLOWED      = 405,
    REQUEST_TIMEOUT         = 408,
    INTERNAL_SERVER_ERROR   = 500,
    NOT_IMPLEMENTED         = 501,
    BAD_GATEWAY             = 502,
    SERVICE_UNAVAILABLE     = 503
};

enum HTTPHeaderType {
    HOST,
    USER_AGENT,
    CONTENT_TYPE,
    CONTENT_LENGTH,
    CONNECTION,
    DATE,
    SERVER,
    SET_COOKIE
};

class HTTPHeader {
public:

    HTTPHeader(HTTPHeaderType type, const String& value) : m_type(type), m_value(value) {}

    HTTPHeaderType getType() const {
        return m_type;
    }

    const String& getValue() const {
        return m_value;
    }

    void setValue(const String& value) {
        m_value = value;
    }

    const char* getHeaderName() const {
        switch (m_type) {
            case HOST:
                return "Host";
            case USER_AGENT:
                return "User-Agent";
            case CONTENT_TYPE:
                return "Content-Type";
            case CONTENT_LENGTH:
                return "Content-Length";
            case CONNECTION:
                return "Connection";
            case DATE:
                return "Date";
            case SERVER:
                return "Server";
            case SET_COOKIE:
                return "Set-Cookie";
            default:
                return "";
        }
    }

private:
    HTTPHeaderType m_type;
    String m_value;
};

class HTTPRequest {

public:
    
    HTTPRequest(HTTPMethod method, const String& path, HTTPVersion version) : m_method(method), m_path(path), m_version(version) {}

    HTTPMethod getMethod() const {
        return m_method;
    }

    const String& getPath() const {
        return m_path;
    }

    HTTPVersion getVersion() const {
        return m_version;
    }

    void addHeader(const HTTPHeader& header) {
        m_headers.push(header);
    }

    void setBody(const String& body) {
        m_body = body;
    }

    const String& getBody() const {
        return m_body;
    }

    const List<HTTPHeader>& getHeaders() const {
        return m_headers;
    }

    /* Header pointers are volatile! */
    const HTTPHeader* getHeader(HTTPHeaderType type) const {
        for (int i = 0; i < m_headers.get_size(); ++i) {
            if (m_headers[i].getType() == type) {
                return &m_headers[i];
            }
        }
        return nullptr;
    }

    void clearHeaders() {
        m_headers.clear();
    }

private:
    HTTPMethod m_method;
    String m_path;
    HTTPVersion m_version;
    List<HTTPHeader> m_headers;
    String m_body;
};

class HTTPCookie {

};

class HTTPContext {

public:
    HTTPContext() {}

    const HTTPCookie& getCookie() const {
        return m_cookie;
    }

private:
    HTTPCookie m_cookie;
};

/* Global counter for unique IDs */
int global_counter = 0;

/* TypeID generation mechanism */
template <typename T>
class TypeID {
public:
    static int get() {
        static int id = new_id();
        return id;
    }

private:
    static int new_id() {
        return global_counter++;
    }
};

class HTTPEngine {

public:

    class HTTPResponse {
    public:
        HTTPResponse() {}
        ~HTTPResponse() {
            for (int i = 0; i < m_headers.get_size(); ++i) {
                delete m_headers[i];
            }
        }
    
        void setStatusCode(HTTPStatusCode code) {
            m_statusCode = code;
        }
    
        HTTPStatusCode getStatusCode() const {
            return m_statusCode;
        }
    
        void addHeader(const HTTPHeader& header) {
            m_headers.push(new HTTPHeader(header));
        }
    
        void setBody(const String& body) {
            m_body = body;
        }

        const String& getBody() const {
            return m_body;
        }
    
    private:
        HTTPStatusCode m_statusCode;
        List<HTTPHeader*> m_headers;
        String m_body;
    
    };

    class Route {
    public:
        Route(const char* path, HTTPMethod method, Function handler) : m_path(path), m_method(method), m_handler(handler) {}
    
        const String& getPath() const {
            return m_path;
        }
    
        HTTPMethod getMethod() const {
            return m_method;
        }

        void call() {
            m_handler();
        }
    
    private:
        String m_path;
        HTTPMethod m_method;
        Function m_handler;
    };

    class Service {
    public:
        Service() = default;
        virtual ~Service() = default;
        virtual void init() = 0;
    private:
    };
    
    enum ServiceLifetime {
        Singleton,
        Transient
    };
    
    /* ServiceContainer class to manage services */
    class ServiceContainer {
    public:

        ServiceContainer() {}

        struct ServiceFactory {
            Service* (*factory)(ServiceContainer* container);
            int type_id;
        };

        /* Method to add a service with specified lifetime */
        template <typename T>
        void addService(ServiceLifetime lifetime) {
            if (m_factory_count < MAX_SERVICES) {
                m_factories[m_factory_count] = (struct ServiceFactory) {
                    [](ServiceContainer* container) -> Service* {
                        return new T(container);
                    },
                    TypeID<T>::get()
                };
            }
            ++m_factory_count;
        }

        /* Method to get a service of type T */
        template <typename T>
        T* getService() {
            for (int i = 0; i < m_factory_count; ++i) {
                if (m_factories[i].type_id == TypeID<T>::get()) {
                    Service* service = m_factories[i].factory(this);
                    return dynamic_cast<T*>(service);
                }
            }
            return nullptr;
        }

        /**
         * Method to get a service of derived type T
         * @note This is an expensive operation as it creates and deletes services
         * @return T* or nullptr if not found
         */
        template <typename T>
        T* getDerivedService() {
            for (int i = 0; i < m_factory_count; ++i) {
                Service* service = m_factories[i].factory(this);
                if (instanceof<T>(service)) {
                    return dynamic_cast<T*>(service);
                }
                delete service;
            }
            return nullptr;
        }

    private:
        static const int MAX_SERVICES = 100; /* Maximum number of services */
        ServiceFactory m_factories[MAX_SERVICES];
        int m_factory_count;
    };

    class Controller : public Service {
    public:
        Controller(ServiceContainer* container) {
            this->container = container;
        }

        /* Destructor */
        virtual ~Controller() {
            for (int i = 0; i < m_routes.get_size(); ++i) {
                delete m_routes[i];
            }
        }
        
        void addRoute(const Route& route) {
            m_routes.push(new Route(route));
        }

        HTTPResponse response;
    protected:
        ServiceContainer* container;
    private:
        List<Route*> m_routes;
    };

    HTTPEngine() {}

    virtual void init() = 0;

    template <typename T>
    void addService(ServiceLifetime lifetime) {
        getServiceContainer()->addService<T>(lifetime);
    }

    template<typename Base, typename T>
    static bool instanceof(const T *ptr) {
        return dynamic_cast<const Base*>(ptr) != nullptr;
    }


    void parseRequest(const String& request) {
        StringList* lines = request.split('\n');
        if(lines->getSize() == 0) {
            return;
        }

        String firstLine = lines->get(0);
        StringList* parts = firstLine.split(' ');
        if(parts->getSize() != 3) {
            return;
        }

        Controller* serv = getServiceContainer()->getDerivedService<Controller>();
        if(serv != nullptr) {
            printf("Service found\n");
            serv->init();
            delete serv;
        }

        String method = parts->get(0);
        String path = parts->get(1);
        String version = parts->get(2);

        printf("Method: %s\n", method.getData());
        printf("Path: %s\n", path.getData());
        printf("Version: %s\n", version.getData());

        HTTPRequest req(parseMethod(method), path, parseVersion(version));

        delete lines;
    }

    static ServiceContainer* getServiceContainer() {
        static ServiceContainer m_serviceContainer;
        return &m_serviceContainer;
    }

private:


    HTTPContext m_context;
    List<Controller> m_controllers;

    HTTPMethod parseMethod(const String& method) {
        if (String::strcmp(method.getData(), "GET") == 0) {
            return GET;
        } else if (String::strcmp(method.getData(), "POST") == 0) {
            return POST;
        } else if (String::strcmp(method.getData(), "PUT") == 0) {
            return PUT;
        } else if (String::strcmp(method.getData(), "DELETE") == 0) {
            return DELETE;
        } else if (String::strcmp(method.getData(), "HEAD") == 0) {
            return HEAD;
        } else if (String::strcmp(method.getData(), "OPTIONS") == 0) {
            return OPTIONS;
        } else if (String::strcmp(method.getData(), "TRACE") == 0) {
            return TRACE;
        } else if (String::strcmp(method.getData(), "CONNECT") == 0) {
            return CONNECT;
        } else if (String::strcmp(method.getData(), "PATCH") == 0) {
            return PATCH;
        } else {
            printf("Unknown method: %s\n", method.getData());
            return GET;
        }
    }

    HTTPVersion parseVersion(const String& version) {
        if (String::strcmp(version.getData(), "HTTP/1.0") == 0) {
            return HTTP_1_0;
        } else if (String::strcmp(version.getData(), "HTTP/1.1") == 0) {
            return HTTP_1_1;
        } else if (String::strcmp(version.getData(), "HTTP/2.0") == 0) {
            return HTTP_2_0;
        } else {
            printf("Unknown version: %s\n", version.getData());
            return HTTP_1_1;
        }
    }

};

#endif /* A708D386_C059_4ABA_94C8_F1D39D13EB36 */
