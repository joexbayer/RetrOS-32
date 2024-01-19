/**
 * @file kevents.c
 * @author Joe Bayer (joexbayer)
 * @brief Kernel events
 * @version 0.1
 * @date 2024-01-19
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <kevents.h>
#include <memory.h>
#include <errors.h>
#include <rtc.h>
#include <terminal.h>

/* ops prototypes */
static int kevents_add(struct kevents *events, kevent_type_t type, const char* fmt, ...);
static int kevents_init(struct kevents *events);
static int kevents_destroy(struct kevents *events);
static int kevents_list(struct kevents *events);

/* ops */
static struct kevents_ops ops = {
    .add = kevents_add,
    .init = kevents_init,
    .destroy = kevents_destroy,
    .list = kevents_list,
};

/* helpers */
static int __kevents_shift(struct kevents *events)
{
    ERR_ON_NULL(events);

    for(size_t i = 0; i < events->count - 1; i++){
        events->events[i] = events->events[i + 1];
    }

    events->count--;

    return 0;
}

static int kevents_init(struct kevents *events)
{
    ERR_ON_NULL(events);

    /* anything more I need to do? */    
    events->count = 0;

    return 0;
}

static int kevents_list(struct kevents *events)
{
    ERR_ON_NULL(events);

    twritef("Kernel Events:\n");
    for(size_t i = 0; i < events->count; i++){
        struct kevent* event = &events->events[i];
        switch(event->type){
        case KEVENT_INFO:
            twritef("[INFO] %s\n", event->info);
            break;
        case KEVENT_WARNING:
            twritef("[WARNING] %s\n", event->info);
            break;
        case KEVENT_ERROR:
            twritef("[ERROR] %s\n", event->info);
            break;
        default:
            twritef("[UNKNOWN] %s\n", event->info);
            break;
        }
    }

    return 0;
}

static int kevents_destroy(struct kevents *events)
{
    ERR_ON_NULL(events);

    kfree(events->events);
    kfree(events);

    return 0;
}

static int kevents_add(struct kevents *events, kevent_type_t type, const char* fmt, ...)
{
    if(events->size == events->count){
        __kevents_shift(events);
    }

    va_list args;
    va_start(args, fmt);

    csprintf(events->events[events->count].info, fmt, args);
    
    va_end(args);

    events->events[events->count].type = type;
    events->events[events->count].timestamp = get_time();
    
    if(events->count < events->size) events->count++;

    return 0;
}

struct kevents* kevents_create(size_t size)
{
    struct kevents* kevents = create(struct kevents);
    ERR_ON_NULL_PTR(kevents);

    kevents->events = (struct kevent*)kalloc(sizeof(struct kevent) * size);
    if(kevents->events == NULL)
    {
        kfree(kevents);
        return NULL;
    }
    kevents->size = size;
    kevents->count = 0;
    kevents->ops = &ops;

    return kevents;   
}