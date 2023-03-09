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
#include <assert.h>

/**
 * @brief Initializes the given lock. Most importantly it sets the blocked list.
 * 
 * @param l Lock to initialize.
 */
void mutex_init(mutex_t* l)
{
    l->pcb_blocked = NULL;
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
        pcb_queue_push(&l->pcb_blocked, current_running);

        block();
        break;
    
    case UNLOCKED:
        l->state = LOCKED;
        break;
    
    default:
        assert(0);
        break;
    }

    //dbgprintf("[SYNC] %s acquired 0x%x\n", current_running->name, l);

    assert(l->state == LOCKED);
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
    struct pcb* blocked = pcb_queue_pop(&l->pcb_blocked);
    if(blocked != NULL){
        pcb_queue_push_running(blocked);
        unblock(blocked->pid);

        assert(l->state == LOCKED);
        STI();
        return;
    }

    //assert(l->state != UNLOCKED);
    l->state = UNLOCKED;
    STI();
}