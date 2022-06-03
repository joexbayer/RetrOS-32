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

/*
    Spinlocks using assembly functions, code from:
    https://stackoverflow.com/questions/6935442/x86-spinlock-using-cmpxchg
*/

void fn(){

}

/**
 * @brief Locks the given spinlock variable l.
 * 
 * @param l lock variable
 */
void spin_lock(int volatile *l)
{
    //while(!__sync_bool_compare_and_swap(l, 0, 1))
    //{
        //do{}while(*l);
    //}
}

/**
 * @brief Unlocks the given spinlock variable l.
 * 
 * @param l lock variable. 
 */
void spin_unlock(int volatile *l)
{
    asm volatile ("":::"memory");
    *l = 0;
}

/**
 * @brief Initializes the given lock. Most importantly it sets the blocked list.
 * 
 * @param l Lock to initialize.
 */
void lock_init(lock_t* l)
{
    for (size_t i = 0; i < MAX_BLOCKED; i++)
    {
        l->blocked[i] = -1;
    }

    l->state = UNLOCKED;
}

inline void __lock_block(lock_t* l)
{
    for (size_t i = 0; i < MAX_BLOCKED; i++)
    {
        if(l->blocked[i] == -1){
            l->blocked[i] = current_running->pid;
            block();
            break;
        }
    }
}

inline int __lock_unlock(lock_t* l)
{
    for (size_t i = 0; i < MAX_BLOCKED; i++)
    {
        if(l->blocked[i] != -1){
            return l->blocked[i];
        }
    }
    return -1;
}


/**
 * @brief Locks the given l and blocks in case its already locked.
 * 
 * @param l lock_t object.
 */
void lock(lock_t* l)
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
        /* TODO: Throw error. */
        break;
    }
    STI();
}

/**
 * @brief Unlocks the given lock, if a process is blocked, unblock it.
 * 
 * @param l Lock to unlock.
 */
void unlock(lock_t* l)
{
    int pid = __lock_unlock(l);
    if(pid != -1){
        unblock(pid);
        return;
    }

    l->state = UNLOCKED;
}