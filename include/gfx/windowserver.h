#ifndef WINDOW_SERVER_H
#define WINDOW_SERVER_H

#include <stdint.h>
#include <kutils.h>

#include <gfx/window.h>
#include <windowmanager.h>

#include <mouse.h>

#define WINDOW_SERVER_SLEEP_TIME 2
#define WS_VALIDATE(ws) if((ws)->ops == NULL || (ws)->ops->add == NULL || (ws)->ops->remove == NULL || (ws)->ops->draw == NULL) { return -ERROR_OPS_CORRUPTED; }
#define WS_VALIDATE_FLAGS(ws) if((ws)->flags & ~(WINDOW_SERVER_INITIALIZED)) { return -ERROR_INVALID_ARGUMENTS; }

typedef enum window_server_flags {
    WINDOW_SERVER_UNINITIALIZED = 1 << 0,
    WINDOW_SERVER_INITIALIZED = 1 << 1
} ws_flag_t;

struct windowserver;

/* window server operations */
struct window_server_ops {
    int (*init)(struct windowserver* ws);
    int (*add)(struct windowserver* ws, struct window* window);
    int (*remove)(struct windowserver* ws, struct window* window);
    int (*fullscreen)(struct windowserver* ws, struct window* window);
    int (*set_background)(struct windowserver* ws, color_t color);
    int (*set_background_file)(struct windowserver* ws, const char* path);
    int (*draw)(struct windowserver* ws);
    int (*destroy)(struct windowserver* ws);
};

/* window server struct */
struct windowserver {
    uint8_t sleep_time;

    /* references for memory management */
    struct kref _krefs;

    struct pcb* server;
    struct pcb* taskbar;

    bool_t _is_fullscreen;

    /* value to hold a pointer while window is in fullscreen. */
    void* _tmp;
    /* frame buffers */
    ubyte_t* background;

    int buffer_size;

    int workspace;

    struct windowmanager* _wm;
    struct window_server_ops* ops;

    /* states */
    struct mouse m;
    struct time time;
    int window_changes;
    
    byte_t flags;
};

/* window server functions */
struct windowserver* ws_new();



#endif /* !WINDOW_SERVER_H */