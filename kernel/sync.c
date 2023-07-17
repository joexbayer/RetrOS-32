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
    WAIT(__sync_lock_test_and_set(lock, SPINLOCK_LOCKED));
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
    struct pcb* current;

    ENTER_CRITICAL();
    switch (l->state){
    case LOCKED:
        current = get_scheduler()->ops->consume(get_scheduler());
        l->blocked->ops->push(l->blocked, current);

        dbgprintf("Locking 0x%x (%d: %s)\n", l, current->pid, current->name);

        get_scheduler()->ops->block(get_scheduler(), current);
        break;  
    
    case UNLOCKED:
        l->state = LOCKED;
        break;
    
    default:
        warningf("Invalid lock state: %d\n", l->state);
        assert(0);
        break;
    }

    assert(l->state == LOCKED);
    LEAVE_CRITICAL();
}

/**
 * @brief Unlocks the given lock, if a process is blocked, unblock it.
 * 
 * @param l Lock to unlock.
 */
void release(mutex_t* l)
{

    ENTER_CRITICAL();
    struct pcb* blocked = l->blocked->ops->pop(l->blocked);
    if(blocked != NULL){
        get_scheduler()->ops->add(get_scheduler(), blocked);
        blocked->state = RUNNING;

        assert(l->state == LOCKED);
        LEAVE_CRITICAL();
        return;
    }
    //assert(l->state != UNLOCKED);
    l->state = UNLOCKED;
    LEAVE_CRITICAL();
}