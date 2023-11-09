#ifndef __WINDOWMANAGER_H
#define __WINDOWMANAGER_H

#include <stdint.h>
#include <sync.h>
#include <gfx/window.h>

#define WM_MAX_WORKSPACES 4

/* macro to validate that all ops in a wm are not NULL */
#define WM_VALIDATE(wm) if((wm)->ops == NULL || (wm)->ops->add == NULL || (wm)->ops->remove == NULL || (wm)->ops->draw == NULL || (wm)->ops->push_front == NULL || (wm)->ops->mouse_event == NULL) { return -ERROR_OPS_CORRUPTED; }
/* validate flags macro */
#define WM_VALIDATE_FLAGS(wm) if((wm)->flags & ~(WM_FULLSCREEN | WM_RESIZABLE)) { return -ERROR_INVALID_ARGUMENTS; }

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
    /* select a workspace */
    int (*workspace)(struct windowmanager *wm, int workspace);

    int (*init)(struct windowmanager *wm, int flags);
    int (*destroy)(struct windowmanager *wm);

    int (*changes)(struct windowmanager *wm);
};

/* Window manager struct */
struct windowmanager {
    uint32_t composition_buffer_size;
    struct windowmanager_ops *ops;
    uint8_t* composition_buffer;
    struct window *windows;

    /* workspaces */
    struct window* workspaces[WM_MAX_WORKSPACES];
    int workspace;

    uint32_t window_count;
    spinlock_t spinlock;

    /* state */
    windowmanager_state_t state;

    struct kref _krefs;

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

struct windowmanager* wm_new(int flags);

int wm_destroy(struct windowmanager* wm);

/* TODO: remove when not needed anymore */
int init_windowmanager(struct windowmanager* wm, int flags);

#endif /* __WINDOWMANAGER_H */
