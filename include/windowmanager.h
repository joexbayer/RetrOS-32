#ifndef __WINDOWMANAGER_H
#define __WINDOWMANAGER_H

#include <stdint.h>
#include <sync.h>
#include <gfx/window.h>

struct windowmanager;
struct windowmanager_ops {
    /* add new window */
    int (*add)(struct windowmanager *wm, struct window *window);
    /* remove window */
    int (*remove)(struct windowmanager *wm, struct window *window);
    /* draw all windows */
    int (*draw)(struct windowmanager *wm, struct window *window);
    int (*push_front)(struct windowmanager *wm, struct window *window);
    int (*mouse_event)(struct windowmanager *wm, int x, int y, char type);
};

/* Window manager struct */
struct windowmanager {
    uint32_t composition_buffer_size;
    struct windowmanager_ops *ops;
    uint8_t* composition_buffer;
    struct window *windows;
    uint32_t window_count;
    spinlock_t spinlock;
    
    int mouse_state;
};

#endif /* __WINDOWMANAGER_H */
