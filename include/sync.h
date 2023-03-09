#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>

#define MAX_BLOCKED 20

enum {
    LOCKED,
    UNLOCKED
};

void spin_lock(int volatile *p);
void spin_unlock(int volatile *p);
typedef int volatile spinlock_t;

typedef struct _mutex {
    int state;
    struct pcb_queue* blocked;
} mutex_t;

void mutex_init(mutex_t* l);
void acquire(mutex_t* l);
void release(mutex_t* l);

/* Assuming that obj has a lock, acquire it and run the code before releasing. */
#define LOCK(obj, code_block) \
    acquire(&obj->lock); \
    do { \
        code_block \
    } while (0);\
    release(&obj->lock); \

#define SPINLOCK(obj, code_block) \
    spin_lock(&obj->spinlock); \
    do { \
        code_block \
    } while (0);\
    spin_unlock(&obj->spinlock); \

#endif