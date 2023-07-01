#ifndef __WINDOWMANAGER_H
#define __WINDOWMANAGER_H

#include <stdint.h>
#include <sync.h>
#include <gfx/window.h>

typedef enum windowmanager_states {
    WM_UNUSED,
    WN_ACTIVE,
    WM_INITIALIZED
} windowmanager_state_t;

typedef enum windowmanager_flags {
    WM_FULLSCREEN = 1 << 0,
    WM_RESIZABLE = 1 << 1,
} windowmanager_flag_t;

struct windowmanager;
struct windowmanager_ops {
    /* add new window */
    int (*add)(struct windowmanager *wm, struct window *window);
    /* remove window */
    int (*remove)(struct windowmanager *wm, struct window *window);
    /* draw all windows */
    int (*draw)(struct windowmanager *wm, struct window *window);
    int (*push_front)(struct windowmanager *wm, struct window *window);
    int (*mouse_event)(struct windowmanager *wm, int x, int y, char flags);
};

/* Window manager struct */
struct windowmanager {
    uint32_t composition_buffer_size;
    struct windowmanager_ops *ops;
    uint8_t* composition_buffer;
    struct window *windows;
    uint32_t window_count;
    spinlock_t spinlock;

    /* state */
    windowmanager_state_t state;

    /* flags */
    windowmanager_flag_t flags;

    struct {
        int x;
        int y;
        char flags;
        char state;
    } mouse;    

    int mouse_state;
};

int init_windowmanager(struct windowmanager* wm, int flags);

#endif /* __WINDOWMANAGER_H */
