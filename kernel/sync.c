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
#include <scheduler.h>
#include <pcb.h>
#include <serial.h>

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
    for (int i = 0; i < MAX_BLOCKED; i++)
    {
        l->blocked[i] = -1;
    }

    l->state = UNLOCKED;
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
        pcb_queue_remove(current_running);
        pcb_queue_push(&l->pcb_blocked, current_running, SINGLE_LINKED);

        //dbgprintf("[SYNC] %s blocked trying to acquire 0x%x\n", current_running->name, l);
        block();
        break;
    
    case UNLOCKED:
        l->state = LOCKED;
        break;
    
    default:
        twriteln("PANIC 2");
        break;
    }

    //dbgprintf("[SYNC] %s acquired 0x%x\n", current_running->name, l);
    STI();
}

/**
 * @brief Unlocks the given lock, if a process is blocked, unblock it.
 * 
 * @param l Lock to unlock.
 */
void release(mutex_t* l)
{

    //dbgprintf("[SYNC] %s released 0x%x\n", current_running->name, l);

    CLI();
    struct pcb* blocked = pcb_queue_pop(&l->pcb_blocked, SINGLE_LINKED);
    if(blocked != NULL){
        pcb_queue_push_running(blocked);
        unblock(blocked->pid);

        //dbgprintf("[SYNC] %s unblocked %s waiting for 0x%x\n", current_running->name, blocked->name, l);
        return;
    }    


    l->state = UNLOCKED;
    STI();
}