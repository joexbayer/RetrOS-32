#ifndef A708D386_C059_4ABA_94C8_F1D39D13EB36
#define A708D386_C059_4ABA_94C8_F1D39D13EB36

#include <lib/http.h>
#include <libc.h>
#include <lib/syscall.h>
#include <fs/fs.h>
#include <utils/StringHelper.hpp>
#include <utils/Web/FileRepository.hpp>

/* C++ helpers that wrap the C http parser/response builder. */

namespace http {

static inline char* dup_cstr(const char* src) {
    if (!src) {
        return nullptr;
    }
    int len = strlen(src);
    char* out = (char*)malloc(len + 1);
    if (!out) {
        return nullptr;
    }
    strcpy(out, src);
    out[len] = '\0';
    return out;
}

class Request {
public:
    Request() {
        reset();
    }

    ~Request() {
        destroy();
    }

    bool parse(const char* raw) {
        destroy();
        int ret = http_parse(raw, &req_);
        parsed_ = (ret == 0);
        return parsed_;
    }

    bool parse(const String& raw) {
        return parse(raw.getData());
    }

    bool parseData() {
        if (!parsed_) return false;
        return http_parse_data(&req_) == 0;
    }

    const http_request& raw() const { return req_; }
    http_request& raw() { return req_; }

    http_method_t method() const { return req_.method; }
    const char* path() const { return req_.path; }
    const char* body() const { return req_.body; }
    int contentLength() const { return req_.content_length; }

    const char* header(const char* key) const {
        return req_.headers ? http_kv_get(req_.headers, key) : nullptr;
    }

    const char* param(const char* key) const {
        return req_.params ? http_kv_get(req_.params, key) : nullptr;
    }

    const char* data(const char* key) const {
        return req_.data ? http_kv_get(req_.data, key) : nullptr;
    }

private:
    void reset() {
        memset(&req_, 0, sizeof(req_));
        parsed_ = false;
    }

    void destroy() {
        if (req_.path) {
            free(req_.path);
        }
        if (req_.body) {
            free(req_.body);
        }
        if (req_.headers) {
            http_kv_destroy(req_.headers, 1);
        }
        if (req_.params) {
            http_kv_destroy(req_.params, 1);
        }
        if (req_.data) {
            http_kv_destroy(req_.data, 1);
        }
        reset();
    }

    http_request req_;
    bool parsed_;
};

class Response {
public:
    Response() {
        reset();
    }

    ~Response() {
        destroy();
    }

    void setStatus(http_error_t status) {
        res_.status = status;
    }

    /**
     * @brief Sets the body of the response.
     * @param body The body content
     * @param length The length of the body. If -1, strlen is used.
     * @warning This function duplicates the body string. Caller is responsible for freeing the original if needed.
     *
     * @return void
     */
    void setBody(const char* body, int length = -1) {
        if (res_body_) {
            free(res_body_);
            res_body_ = nullptr;
        }

        if (!body) {
            res_.body = nullptr;
            res_.content_length = 0;
            return;
        }

        size_t len = (length >= 0) ? (size_t)length : strlen(body);
        res_body_ = (char*)malloc(len + 1);
        if (!res_body_) {
            res_.body = nullptr;
            res_.content_length = 0;
            return;
        }

        memcpy(res_body_, body, len);
        res_body_[len] = '\0';
        res_.body = res_body_;
        res_.content_length = len;
    }

    /**
     * @brief Convenience helper to load a file into the response body.
     * @param path Path to the file on the filesystem.
     * @param contentType Optional content type header to attach.
     * @return true if the file was loaded and attached, false otherwise.
     */
    bool sendFile(const char* path, const char* contentType = "text/html") {
        static const size_t MAX_FILE_SIZE = 6 * 1024; /* Keep responses within the 8KB buffer. */
        char* buffer = (char*)malloc(MAX_FILE_SIZE);
        if (!buffer) {
            setStatus(HTTP_500_INTERNAL_SERVER_ERROR);
            setBody("Internal Server Error");
            return false;
        }

        int fd = open(path, FS_FILE_FLAG_READ);
        if (fd < 0) {
            free(buffer);
            setNotFound(contentType);
            return false;
        }

        int size = read(fd, buffer, MAX_FILE_SIZE);
        fclose(fd);

        if (size <= 0) {
            free(buffer);
            setNotFound(contentType);
            return false;
        }

        setStatus(HTTP_200_OK);
        setBody(buffer, size);
        free(buffer);

        if (contentType) {
            addHeader("Content-Type", contentType);
        }
        return true;
    }

    bool sendFile(web::FileRepository& repo, const char* path, const char* contentType = "text/html") {
        web::FileData fileData = repo.getFile(path);
        if (fileData.content) {
            setStatus(HTTP_200_OK);
            setBody(fileData.content, fileData.size);
            if (contentType) {
                addHeader("Content-Type", contentType);
            }
            return true;
        }
        return sendFile(path, contentType);
    }

    void addHeader(const char* key, const char* value) {
        if (!res_.headers) {
            res_.headers = http_kv_create(16);
        }
        if (!res_.headers) {
            return;
        }
        char* vdup = dup_cstr(value);
        if (!vdup) {
            return;
        }
        if (http_kv_insert(res_.headers, key, vdup) != 0) {
            free(vdup);
        }
    }

    int build(char* buffer, size_t buffer_size) const {
        return http_build_response(&res_, buffer, buffer_size);
    }

    const http_response& raw() const { return res_; }
    http_response& raw() { return res_; }

private:
    void reset() {
        memset(&res_, 0, sizeof(res_));
        res_.headers = nullptr;
        res_body_ = nullptr;
    }

    void destroy() {
        if (res_.headers) {
            http_kv_destroy(res_.headers, 1);
            res_.headers = nullptr;
        }
        if (res_body_) {
            free(res_body_);
            res_body_ = nullptr;
        }
        reset();
    }

    void setNotFound(const char* contentType) {
        setStatus(HTTP_404_NOT_FOUND);
        setBody("<h1>404 Not Found</h1><p>The requested file was not found on the server.</p>");
        if (contentType) {
            addHeader("Content-Type", contentType);
        }
    }

    http_response res_;
    char* res_body_;
};

class HttpEngine {
public:
    static bool Parse(const char* raw, Request& out) {
        return out.parse(raw);
    }

    static bool Parse(const String& raw, Request& out) {
        return out.parse(raw);
    }

    static bool ParseData(Request& req) {
        return req.parseData();
    }

    static int BuildResponse(const Response& res, char* buffer, size_t buffer_size) {
        return res.build(buffer, buffer_size);
    }
};

}

#endif /* A708D386_C059_4ABA_94C8_F1D39D13EB36 */
