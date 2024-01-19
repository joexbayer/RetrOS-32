#ifndef __KEVENTS_H__
#define __KEVENTS_H__

#include <stdint.h>
#include <kernel.h>
#include <kutils.h>

typedef enum {
    KEVENT_INFO,
    KEVENT_WARNING,
    KEVENT_ERROR,
} kevent_type_t;

/* prototype */
struct kevents;

struct kevents_ops {
    int (*init)(struct kevents *events);
    
    /**
     * @brief Add a new event to the list 
     * @param event kevent_type_t 
     * @param fmt Format string for info, max 128 chars
     * 
     * @return int
     */
    int (*add)(struct kevents *events, kevent_type_t type, const char* fmt, ...);
    int (*list)(struct kevents *events);
    int (*destroy)(struct kevents *events);
};

/* main kevents struct */
struct kevents {
    struct kevents_ops *ops;
    struct kevent {
        kevent_type_t type;
        char info[128];
        uint32_t timestamp;
    }* events;
    uint32_t count;
    size_t size;
    
};

struct kevents* kevents_create(size_t size);

#endif // !__KEVENTS_H__