#ifndef A708D386_C059_4ABA_94C8_F1D39D13EB36
#define A708D386_C059_4ABA_94C8_F1D39D13EB36

class HTTPHeader {
public:
    char* name;
    char* value;

    char* toString();
};

class HTTPCookie {

};

class HTTPContext {

};

class HTTPRequest {
public:
    char* method;
    char* path;
    HTTPHeader* headers;
    int headerCount;
    char* body;
};

class HttpResponse {
    int status_code;
    char* status_message;
    HTTPHeader* headers;
    int headerCount;
    char* body;
};

void freeHTTPRequest(HTTPRequest& request) {
    delete[] request.method;
    delete[] request.path;

    for (int i = 0; i < request.headerCount; i++) {
        delete[] request.headers[i].name;
        delete[] request.headers[i].value;
    }
    delete[] request.headers;

    delete[] request.body;
}


#endif /* A708D386_C059_4ABA_94C8_F1D39D13EB36 */
