#include <scheduler.h>
#include <memory.h>
#include <timer.h>
#include <serial.h>
#include <assert.h>
#include <work.h>

#include <arch/gdt.h>
#include <arch/tss.h>

/* exposed operator functions */
static int sched_prioritize(struct scheduler* sched, struct pcb* pcb);
static int sched_default(struct scheduler* sched);
static int sched_add(struct scheduler* sched, struct pcb* pcb);
static int sched_block(struct scheduler* sched, struct pcb* pcb);
static int sched_sleep(struct scheduler* sched, int time);
static struct pcb* sched_consume(struct scheduler* sched);
static int sched_exit(struct scheduler* sched);

static int sched_round_robin(struct scheduler* sched);

/* Default scheduler operations */
static struct scheduler_ops sched_default_ops = {
    .prioritize = &sched_prioritize,
    .add = &sched_add,
    .schedule = &sched_default,
    .sleep = &sched_sleep,
    .exit = &sched_exit,
    .yield = &sched_default,
    .consume = &sched_consume,
    .block = &sched_block
};

/* Default scheduler instance */
static struct scheduler sched_default_instance = {
    .ops = &sched_default_ops,
    .queue = NULL,
    .priority = NULL,
    .ctx.running = NULL,
    .yields = 0,
    .exits = 0,
    .flags = SCHED_UNUSED
};

/**
 * @brief Initializes the default scheduler
 * Sets up the default scheduler with the given flags.
 * Using default operations and creates queues.
 * @param sched  The scheduler to initialize
 * @param flags  Flags to set on the scheduler
 * @return error_t  0 on success, error code on failure
 */
error_t sched_init_default(struct scheduler* sched, sched_flag_t flags)
{
    ERR_ON_NULL(sched);
    if(sched->flags & SCHED_INITIATED){
        return -ERROR_SCHED_EXISTS;
    }
    
    sched->ops = &sched_default_ops;

    sched->priority = pcb_new_queue();
    if(sched->priority == NULL){
        return -ERROR_PCB_QUEUE_CREATE;
    }

    sched->queue = pcb_new_queue();
    if(sched->queue == NULL){
        return -ERROR_PCB_QUEUE_CREATE;
    }

    sched->flags = flags | SCHED_INITIATED;

    return ERROR_OK;
}

/**
 * @brief Puts the current running process to sleep for the given time
 * 
 * @param sched  The scheduler to sleep on
 * @param time  The time to sleep for
 * @return int  0 on success, error code on failure
 */
static int sched_sleep(struct scheduler* sched, int time)
{
    ERR_ON_NULL(sched);
    SCHED_VALIDATE(sched);

    assert(sched->ctx.running != NULL);

    sched->ctx.running->sleep = timer_get_tick() + time;
    sched->ctx.running->state = SLEEPING;

    (void)sched->ops->schedule(sched);

    return ERROR_OK;
}

/**
 * @brief Prioritizes the given pcb in the scheduler
 * 
 * @param sched  The scheduler to prioritize on
 * @param pcb  The pcb to prioritize
 * @return int  0 on success, error code on failure
 */
static int sched_prioritize(struct scheduler* sched, struct pcb* pcb)
{
    ERR_ON_NULL(sched);
    ERR_ON_NULL(pcb);
    SCHED_VALIDATE(sched);

    /* Add pcb to priority queue */
    RETURN_ON_ERR(sched->priority->ops->push(sched->priority, pcb));

    return ERROR_OK;
}

/**
 * @brief Consumes the current running pcb
 * This function is useful if a process wants to take ownership of the current running pcb.
 * @warning The scheduler will have no running pcb after this function is called.
 * @param sched  The scheduler to consume on
 * @return struct pcb*  The consumed pcb
 */
static struct pcb* sched_consume(struct scheduler* sched)
{
    ERR_ON_NULL_PTR(sched);

    /* Consume the current running pcb and return it */ 
    struct pcb* pcb = sched->ctx.running;
    sched->ctx.running = NULL;
    return pcb;
}

/**
 * @brief Blocks the given pcb
 * 
 * @param sched  The scheduler to block on
 * @param pcb  The pcb to block
 * @return int  0 on success, error code on failure
 */
static int sched_block(struct scheduler* sched, struct pcb* pcb)
{
    ERR_ON_NULL(sched);
    ERR_ON_NULL(pcb);
    SCHED_VALIDATE(sched);

    /* At this point current running should have been consumed? */
    assert(sched->ctx.running == NULL);

    pcb->state = BLOCKED;

    CRITICAL_SECTION({
        pcb_save_context(pcb);

        /* Switch to next PCB, should be chosen by flag? */
        assert(sched_round_robin(sched) == ERROR_OK);

        pcb_restore_context(sched->ctx.running);
    });

    return ERROR_OK;
}

/**
 * @brief round robin scheduler
 * 
 * @param sched  The scheduler to schedule on
 * @return int 0 on success, error code on failure
 */
static int sched_round_robin(struct scheduler* sched)
{
    struct pcb* next;
    
    ERR_ON_NULL(sched);
    SCHED_VALIDATE(sched);
    ASSERT_CRITICAL();

    /* If queue is empty, return error */
    next = sched->queue->ops->peek(sched->queue); 
    if(next == NULL){
        warningf("Queue is empty");
        return -ERROR_PCB_QUEUE_EMPTY;
    }

    /* Add current running context to queue */
    if(sched->ctx.running != NULL){
        sched->queue->ops->push(sched->queue, sched->ctx.running);
        sched->ctx.running = NULL;
    }

    /**
     * @brief Loops through the queue until it finds a pcb that is ready to run.
     * Multiple iterations should be rare, athe first pcb will most likely be ready to run.
     * Uses a switch case for all relevant states.
     */
    do {
        next = sched->queue->ops->pop(sched->queue);

        switch (next->state){
        case RUNNING:
            break;
        case PCB_NEW:{
                /**
                 * @brief This is where the new process is started
                 * This calls the start_pcb function and sets up the page directory.
                 * Should only be called once for each pcb.
                 */

                if(next->is_process){
                    tss.esp_0 = (uint32_t)next->kebp;
                    tss.ss_0 = GDT_KERNEL_DS;
                }

                sched->ctx.running = next;
                current_running = next;
                load_page_directory(next->page_dir);
                //load_data_segments(GDT_KERNEL_DS);
                start_pcb(next);
                kernel_panic("Illegal return of 'start_pcb'");/* not sure if it should return or break */
            }
            break; /* Never reached. */
        case ZOMBIE:{
                /**
                 * @brief A PCB is in the ZOMBIE state if it has been killed by another process
                 * or has exited. This is where the PCB is cleaned up.
                 * The ZOMBIE pcb will not be scheduled again and a work thread will deal with cleaning up the pcb.
                 * This is because we want to spend as little time as possible in the scheduler.
                 */
                work_queue_add(&pcb_cleanup_routine, (void*)((int)next->pid), NULL);
            }
            break;
        /* If next is sleeping, check if it should be woken up */
        case SLEEPING:{
                /**
                 * @brief When a pcb is sleeping, we need to know if we should wake it up.
                 * If the pcb's sleep time is less than the current tick we can wake it up
                 * and schedule it as running, else it will be put at the end of the queue.
                 */
                if(next->sleep < timer_get_tick()){
                    next->state = RUNNING;
                    break;
                }
            }
        /* fall through */
        case BLOCKED:
            /* Blocked just means we want to add it back to the queue */
        default:
            /* push next back into queue, should be very rare. */
            sched->queue->ops->push(sched->queue, next);
            break;
        }
    } while(next->state != RUNNING);
    
    sched->ctx.running = next;
    current_running = next;

    if(next->is_process){
        tss.esp_0 = (uint32_t)next->kebp;
        tss.ss_0 = GDT_KERNEL_DS;
    }

    load_page_directory(sched->ctx.running->page_dir);
    return ERROR_OK;
}

/* Default round robin scheduler behavior */
static int sched_default(struct scheduler* sched)
{
    ERR_ON_NULL(sched);
    SCHED_VALIDATE(sched);

    /* If no running process, get one from queue */
    if (sched->ctx.running == NULL){
        sched->ctx.running = sched->queue->ops->pop(sched->queue);
        /* Temporary fix */
        current_running = sched->ctx.running;
    }
    
    sched->ctx.running->yields++;

    CRITICAL_SECTION({

        pcb_save_context(sched->ctx.running);

        /* Switch to next PCB, should be chosen by flag? */
        PANIC_ON_ERR(sched_round_robin(sched));

        pcb_restore_context(sched->ctx.running);
        
        //dbgprintf("Switching too PCB %s with page dir: %x, stack: %x, kstack: %x\n", sched->ctx.running->name, sched->ctx.running->page_dir, sched->ctx.running->ctx.esp, sched->ctx.running->kesp);
    });

    return ERROR_OK;
}

/**
 * @brief Cleans up the given pcb
 * A work is added to complete the pcb cleanup routine and a new process
 * is scheduled. 
 * @param sched The scheduler to clean up on
 * @return int 0 on success, error code on failure
 */
static int sched_exit(struct scheduler* sched)
{
    ERR_ON_NULL(sched);
    SCHED_VALIDATE(sched);

    sched->exits++;  

    /* Add cleanup routine to work queue */
    //work_queue_add(&pcb_cleanup_routine, (void*)((int)sched->ctx.running->pid), NULL);
    sched->ctx.running->state = ZOMBIE;
    
    CRITICAL_SECTION({

        pcb_save_context(sched->ctx.running);

        /* Switch to next PCB, dont need to store context */
        PANIC_ON_ERR(sched_round_robin(sched));

        pcb_restore_context(sched->ctx.running);
    });


    return ERROR_OK;
}

/**
 * @brief Adds the given pcb to the scheduler
 * 
 * @param sched  The scheduler to add to
 * @param pcb The pcb to add
 * @return int 0 on success, error code on failure
 */
static int sched_add(struct scheduler* sched, struct pcb* pcb)
{
    SCHED_VALIDATE(sched);

    RETURN_ON_ERR(sched->queue->ops->push(sched->queue, pcb));
    
    return ERROR_OK;
}

struct scheduler* get_scheduler()
{
    return &sched_default_instance;
}

/* Kernel scheduling API */

void kernel_sleep(int time)
{
    get_scheduler()->ops->sleep(get_scheduler(), time);
}

void kernel_yield()
{   
    assert(get_scheduler()->ops->schedule(get_scheduler()) == 0);
}

void kernel_exit()
{
    get_scheduler()->ops->exit(get_scheduler());

    UNREACHABLE();
}

void block()
{
    struct pcb* current = get_scheduler()->ops->consume(get_scheduler());
    get_scheduler()->ops->block(get_scheduler(), current);
}

void unblock(int pid)
{
    //pcb_set_running(pid);
}

