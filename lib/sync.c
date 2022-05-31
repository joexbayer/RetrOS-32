#include <sync.h>

/*
    Spinlocks using assembly functions, code from:
    https://stackoverflow.com/questions/6935442/x86-spinlock-using-cmpxchg
*/

/**
 * @brief Locks the given spinlock variable l.
 * 
 * @param l lock variable
 */
void spin_lock(int volatile *l)
{
    while(!__sync_bool_compare_and_swap(l, 0, 1))
    {
        do{}while(*l);
    }
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

void lock_init(lock_t* l)
{
    for (size_t i = 0; i < MAX_BLOCKED; i++)
    {
        l->blocked[i] = -1;
    }

    l->state = UNLOCKED;
}

inline void _lock_block(lock_t* l)
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

inline int _lock_check_block(lock_t* l)
{
    for (size_t i = 0; i < MAX_BLOCKED; i++)
    {
        if(l->blocked[i] != -1){
            return l->blocked[i];
        }
    }
    return -1;
}



void lock(lock_t* l)
{
    CLI();
    switch (l->state)
    {
    case LOCKED:
        _lock_block(l);
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

void unlock(lock_t* l)
{
    int pid = _lock_check_block(l);
    if(pid != -1){
        unblock(pid);
        return;
    }

    l->state = UNLOCKED;
}