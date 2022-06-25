#ifndef SYNC_H
#define SYNC_H

#include <pcb.h>
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
    int blocked[MAX_BLOCKED]; /* List of PIDs blocked.*/
    int state;
} mutex_t;

void mutex_init(mutex_t* l);
void acquire(mutex_t* l);
void release(mutex_t* l);

#endif