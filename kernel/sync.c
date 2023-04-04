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

void spin_lock(spinlock_t* lock) {
    WAIT(__sync_lock_test_and_set(lock, 1));
}

void spin_unlock(spinlock_t* lock) {
    __sync_lock_release(lock);
}

/**
 * @brief Initializes the given lock. Most importantly it sets the blocked list.
 * 
 * @param l Lock to initialize.
 */
void mutex_init(mutex_t* l)
{
    l->blocked = pcb_new_queue();
    l->state = UNLOCKED;
    dbgprintf("Lock 0x%x initiated by %s\n", l, current_running->name);
}


/**
 * @brief Locks the given l and blocks in case its already locked.
 * 
 * @param l mutex_t object.
 */
void acquire(mutex_t* l)
{
    CLI();
    switch (l->state){
    case LOCKED:
        dbgprintf("Locking 0x%x (%d: %s )\n", l, current_running->pid, current_running->name);
        pcb_queue_remove_running(current_running);
        l->blocked->ops->push(l->blocked, current_running);

        block();
        break;
    
    case UNLOCKED:
        l->state = LOCKED;
        break;
    
    default:
        assert(0);
        break;
    }

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

    CLI();
    struct pcb* blocked = l->blocked->ops->pop(l->blocked);
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