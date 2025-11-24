#include <mocks.h>
#include <test.h>
#include <lib/http.h>
#include <libc.h>

void free(void *ptr);

static int contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) {
        return 0;
    }
    int hlen = strlen(haystack);
    int nlen = strlen(needle);
    if (nlen == 0 || hlen < nlen) {
        return 0;
    }
    for (int i = 0; i <= hlen - nlen; i++) {
        if (strncmp(haystack + i, needle, nlen) == 0) {
            return 1;
        }
    }
    return 0;
}

static void cleanup_request(struct http_request *req) {
    if (req->path) {
        free(req->path);
    }
    if (req->body) {
        free(req->body);
    }
    if (req->headers) {
        http_kv_destroy(req->headers, 1);
    }
    if (req->params) {
        http_kv_destroy(req->params, 1);
    }
    if (req->data) {
        http_kv_destroy(req->data, 1);
    }
}

static void test_basic_get_parse() {
    const char *raw =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == 0, "basic GET parse succeeds");
    testprintf(req.method == HTTP_GET, "method parsed as GET");
    testprintf(req.version == HTTP_VERSION_1_1, "HTTP version parsed");
    testprintf(strcmp(req.path, "/index.html") == 0, "path parsed");
    testprintf(req.keep_alive == 1 && req.close == 0, "connection keep-alive parsed");
    testprintf(strcmp(http_kv_get(req.headers, "Host"), "example.com") == 0, "Host header stored");

    cleanup_request(&req);
}

static void test_query_params() {
    const char *raw =
        "GET /search?q=test&lang=en HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == 0, "query parse succeeds");
    testprintf(strcmp(req.path, "/search") == 0, "query path stripped of params");
    testprintf(http_kv_size(req.params) == 2, "query param count");
    testprintf(strcmp(http_kv_get(req.params, "q"), "test") == 0, "query param q");
    testprintf(strcmp(http_kv_get(req.params, "lang"), "en") == 0, "query param lang");

    cleanup_request(&req);
}

static void test_urlencoded_body_parse() {
    const char *raw =
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 17\r\n"
        "\r\n"
        "name=joe&age=30";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);
    testprintf(ret == 0, "POST parse succeeds");
    testprintf(req.content_length == 17, "content-length parsed");
    testprintf(req.body != NULL, "body captured");

    ret = http_parse_data(&req);
    testprintf(ret == 0, "urlencoded body parsed");
    testprintf(http_kv_size(req.data) == 2, "form field count");
    testprintf(strcmp(http_kv_get(req.data, "name"), "joe") == 0, "form field name");
    testprintf(strcmp(http_kv_get(req.data, "age"), "30") == 0, "form field age");

    cleanup_request(&req);
}

static void test_http_1_0_version() {
    const char *raw =
        "GET / HTTP/1.0\r\n"
        "Host: example.com\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == 0, "HTTP/1.0 parse succeeds");
    testprintf(req.version == HTTP_VERSION_1_0, "HTTP/1.0 detected");

    cleanup_request(&req);
}

static void test_missing_host_rejected() {
    const char *raw =
        "GET / HTTP/1.1\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == -1, "parse fails without Host header");
    testprintf(req.status == HTTP_400_BAD_REQUEST, "status set to 400 Bad Request");

    cleanup_request(&req);
}

static void test_connection_close_and_defaults() {
    const char *raw =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == 0, "connection close parse succeeds");
    testprintf(req.close == 1 && req.keep_alive == 0, "connection close parsed");
    testprintf(req.content_length == 0, "content-length defaults to 0 when missing");

    cleanup_request(&req);
}

static void test_path_too_long_rejected() {
    char path[270];
    for (int i = 0; i < 260; i++) {
        path[i] = 'a';
    }
    path[260] = '\0';

    char raw[400];
    strcpy(raw, "GET /");
    strcat(raw, path);
    strcat(raw, " HTTP/1.1\r\nHost: example.com\r\n\r\n");

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == -1, "overlong path rejected");
    testprintf(req.status == HTTP_414_URI_TOO_LONG, "status set to 414 on long path");

    cleanup_request(&req);
}

static void test_unknown_method_rejected() {
    const char *raw =
        "FOO / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == -1, "unknown method rejected");
    testprintf(req.method == -1, "method marked invalid");

    cleanup_request(&req);
}

static void test_header_case_keep_alive() {
    const char *raw =
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: Keep-Alive\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == 0, "mixed-case Connection parsed");
    testprintf(req.keep_alive == 1 && req.close == 0, "keep-alive recognized case-insensitive");

    cleanup_request(&req);
}

static void test_params_absent() {
    const char *raw =
        "GET /plain HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);

    testprintf(ret == 0, "plain path parse succeeds");
    testprintf(http_kv_size(req.params) == 0, "no query params when absent");

    cleanup_request(&req);
}

static void test_parse_data_without_content_type() {
    const char *raw =
        "POST /empty HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);
    testprintf(ret == 0, "POST without content-type parses");

    ret = http_parse_data(&req);
    testprintf(ret == 0, "parse_data tolerates missing content-type");
    testprintf(req.data != NULL && http_kv_size(req.data) == 0, "data map created empty");

    cleanup_request(&req);
}

static void test_kv_store_capacity_and_duplicates() {
    struct http_kv_store *store = http_kv_create(3);
    testprintf(store != NULL, "kv store created");

    int r1 = http_kv_insert(store, "a", "1");
    int r2 = http_kv_insert(store, "b", "2");
    int r3 = http_kv_insert(store, "b", "other");
    int r4 = http_kv_insert(store, "c", "3");
    int r5 = http_kv_insert(store, "d", "4");

    testprintf(r1 == 0 && r2 == 0, "kv store inserts within capacity");
    testprintf(r3 == 0, "duplicate key ignored without size change");
    testprintf(r4 == 0, "insert up to capacity succeeds");
    testprintf(r5 == -1, "insert rejected when capacity exceeded");
    testprintf(http_kv_size(store) == 3, "kv store size reflects unique keys");

    http_kv_destroy(store, 0);
}

static void test_multipart_form_data() {
    const char *raw =
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: multipart/form-data; boundary=BOUNDARY\r\n"
        "\r\n"
        "--BOUNDARY\r\n"
        "Content-Disposition: form-data; name=\"file\"\r\n"
        "\r\n"
        "hello\r\n"
        "--BOUNDARY--";

    struct http_request req = {0};
    int ret = http_parse(raw, &req);
    testprintf(ret == 0, "multipart parse succeeds");

    ret = http_parse_data(&req);
    testprintf(ret == 0, "multipart body parsed");
    testprintf(strcmp(http_kv_get(req.data, "file"), "hello") == 0, "multipart field extracted");

    cleanup_request(&req);
}

static void test_build_response_defaults() {
    char buffer[256];
    struct http_response res = {0};
    res.status = HTTP_200_OK;
    res.headers = http_kv_create(2);
    res.body = "OK";
    res.content_length = 0;

    int len = http_build_response(&res, buffer, sizeof(buffer));
    testprintf(len > 0, "build response succeeds");
    testprintf(contains(buffer, "HTTP/1.1 200 OK"), "status line present");
    testprintf(contains(buffer, "Content-Length: 2"), "content-length inserted");
    testprintf(contains(buffer, "\r\n\r\nOK"), "body appended");

    http_kv_destroy(res.headers, 0);
}

static void test_build_response_with_headers() {
    char buffer[256];
    struct http_response res = {0};
    res.status = HTTP_302_FOUND;
    res.headers = http_kv_create(4);
    http_kv_insert(res.headers, "Content-Type", "text/plain");
    http_kv_insert(res.headers, "Location", "/redirect");
    res.body = "redir";

    int len = http_build_response(&res, buffer, sizeof(buffer));
    testprintf(len > 0, "build response with headers succeeds");
    testprintf(contains(buffer, "Content-Type: text/plain"), "custom header kept");
    testprintf(contains(buffer, "Location: /redirect"), "second header kept");
    testprintf(contains(buffer, "Content-Length: 5"), "content length matches body");

    http_kv_destroy(res.headers, 0);
}

static void test_build_response_respects_existing_length() {
    char buffer[256];
    struct http_response res = {0};
    res.status = HTTP_200_OK;
    res.headers = http_kv_create(3);
    http_kv_insert(res.headers, "Content-Length", "99");
    res.body = "hi";
    res.content_length = 2;

    int len = http_build_response(&res, buffer, sizeof(buffer));
    testprintf(len > 0, "build response with provided content-length succeeds");
    testprintf(contains(buffer, "Content-Length: 99"), "existing content-length preserved");

    http_kv_destroy(res.headers, 0);
}

static void test_build_response_small_buffer() {
    char buffer[16];
    struct http_response res = {0};
    res.status = HTTP_200_OK;
    res.body = "too long for buffer";
    res.content_length = 0;

    int len = http_build_response(&res, buffer, sizeof(buffer));
    testprintf(len == -1, "build response fails on small buffer");
}

int main() {
    test_basic_get_parse();
    test_query_params();
    test_urlencoded_body_parse();
    test_http_1_0_version();
    test_missing_host_rejected();
    test_connection_close_and_defaults();
    test_path_too_long_rejected();
    test_unknown_method_rejected();
    test_header_case_keep_alive();
    test_params_absent();
    test_parse_data_without_content_type();
    test_kv_store_capacity_and_duplicates();
    test_multipart_form_data();
    test_build_response_defaults();
    test_build_response_with_headers();
    test_build_response_respects_existing_length();
    test_build_response_small_buffer();

    test_summary();
    return 0;
}
