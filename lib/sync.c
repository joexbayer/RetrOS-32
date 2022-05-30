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