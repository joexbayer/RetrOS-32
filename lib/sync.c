/**
 * @file sync.c
 * @author Joe Bayer (joexbayer)
 * @brief Synchronization primitives used for shared resources.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <sync.h>
#include <terminal.h>

/* Fucntions defined in kernel_entry.s */
void spin_lock_asm(int volatile *l);
void spin_unlock_asm(int volatile *l);

/**
 * @brief Locks the given spinlock variable l.
 * 
 * @param l lock variable
 */
void spin_lock(int volatile *l)
{
    spin_lock_asm(l);
}

/**
 * @brief Unlocks the given spinlock variable l.
 * 
 * @param l lock variable. 
 */
void spin_unlock(int volatile *l)
{
    spin_unlock_asm(l);
}

/**
 * @brief Initializes the given lock. Most importantly it sets the blocked list.
 * 
 * @param l Lock to initialize.
 */
void mutex_init(mutex_t* l)
{
    for (size_t i = 0; i < MAX_BLOCKED; i++)
    {
        l->blocked[i] = -1;
    }

    l->state = UNLOCKED;
}

inline void __lock_block(mutex_t* l)
{
    for (size_t i = 0; i < MAX_BLOCKED; i++)
    {
        if(l->blocked[i] == -1){
            l->blocked[i] = current_running->pid;
            //twritef("Blocking %d\n", current_running->pid);
            block();
            return;
        }
    }

    twriteln("PANIC 1");
}

inline int __lock_unlock(mutex_t* l)
{
    for (size_t i = 0; i < MAX_BLOCKED; i++)
    {
        if(l->blocked[i] != -1){
            int ret = l->blocked[i];
            l->blocked[i] = -1;
            return ret;
        }
    }
    return -1;
}


/**
 * @brief Locks the given l and blocks in case its already locked.
 * 
 * @param l mutex_t object.
 */
void acquire(mutex_t* l)
{
    CLI();
    switch (l->state)
    {
    case LOCKED:
        __lock_block(l);
        break;
    
    case UNLOCKED:
        l->state = LOCKED;
        break;
    
    default:
        twriteln("PANIC 2");
        break;
    }
    STI();
}

/**
 * @brief Unlocks the given lock, if a process is blocked, unblock it.
 * 
 * @param l Lock to unlock.
 */
void release(mutex_t* l)
{
    int pid = __lock_unlock(l);
    if(pid != -1){
        //twritef("Unblocking %d\n", pid);
        unblock(pid);
        return;
    }

    l->state = UNLOCKED;
}