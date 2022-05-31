#ifndef SYNC_H
#define SYNC_H

#include <pcb.h>
#include <util.h>

#define MAX_BLOCKED 10

enum {
    LOCKED,
    UNLOCKED
};

void spin_lock(int volatile *p);
void spin_unlock(int volatile *p);
typedef int volatile spinlock_t;

typedef struct _lock {
    int blocked[MAX_BLOCKED]; /* List of PIDs blocked.*/
    int state;
} lock_t;

void lock_init(lock_t* l);
void lock(lock_t* l);
void unlock(lock_t* l);

#endif