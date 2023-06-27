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
    .flags = 0
};

/**
 * @brief Initializes the default scheduler
 * This scheduler is a round robin scheduler, for now...
 * @param sched  The scheduler to initialize
 * @param flags  Flags to set on the scheduler
 * @return error_t  0 on success, error code on failure
 */
error_t sched_init_default(struct scheduler* sched, sched_flag_t flags)
{
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
    dbgprintf("Flags %d\n",sched->flags);

    return 0;
}

void init_test_scheduler()
{
    sched_init_default(&sched_default_instance, 0);
}

struct scheduler* get_default_scheduler()
{
    return &sched_default_instance;
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
    SCHED_VALIDATE(sched);

    sched->ctx.running->sleep = timer_get_tick() + time;
    sched->ctx.running->state = SLEEPING;

    (void)sched->ops->schedule(sched);

    return 0;
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
    SCHED_VALIDATE(sched);

    /* Add pcb to priority queue */
    RETURN_ON_ERR(sched->priority->ops->push(sched->priority, pcb));

    return 0;
}

/**
 * @brief Consumes the current running pcb
 * 
 * @param sched  The scheduler to consume on
 * @return struct pcb*  The consumed pcb
 */
static struct pcb* sched_consume(struct scheduler* sched)
{
    SCHED_VALIDATE(sched);

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
    SCHED_VALIDATE(sched);
    /* At this point current running should have been consumed? */
    assert(sched->ctx.running == NULL);

    pcb->state = BLOCKED;

    CRITICAL_SECTION({
        pcb_save_context(pcb);

        /* Switch to next PCB, should be chosen by flag? */
        assert(sched_round_robin(sched) == 0);

        pcb_restore_context(sched->ctx.running);
    });

    return 0;
}

/**
 * @brief round robin scheduler
 * 
 * @param sched  The scheduler to schedule on
 * @return int 0 on success, error code on failure
 */
static int sched_round_robin(struct scheduler* sched)
{
    dbgprintf("%x\n", sched);
    SCHED_VALIDATE(sched);
    struct pcb* next;
    ASSERT_CRITICAL();

    assert(sched->queue->_list != NULL);

    /* If queue is empty, return error */
    next = sched->queue->ops->peek(sched->queue); 
    if(next == NULL){
        return -ERROR_PCB_QUEUE_EMPTY;
    }

    /* Add current running context to queue */
    if(sched->ctx.running != NULL){
        sched->queue->ops->push(sched->queue, sched->ctx.running);
        sched->ctx.running = NULL;
    }

    /* get next pcb from queue */
    do {
        next = sched->queue->ops->pop(sched->queue); 
        dbgprintf("next %s\n", next->name);

        switch (next->state){
        case PCB_NEW:{
                dbgprintf("Running new PCB %s (PID %d) with page dir: %x: stack: %x kstack: %x\n",
                    next->name,
                    next->pid,
                    next->page_dir,
                    next->ctx.esp,
                    next->kesp
                );

                if(next->is_process){
                    tss.esp_0 = (uint32_t)next->kebp;
                    tss.ss_0 = GDT_KERNEL_DS;
                }

                sched->ctx.running = next;
                current_running = next;
                load_page_directory(next->page_dir);
                //load_data_segments(GDT_KERNEL_DS);
                start_pcb(next);
                return 0; /* not sure if it should return or break */
            }
            break; /* Never reached. */
        case RUNNING:
            break;
        /* If next is sleeping, check if it should be woken up */
        case SLEEPING:{
                if(next->sleep < timer_get_tick()){
                    next->state = RUNNING;
                    break;
                }
            }
        case BLOCKED:
        default:
            /* push next back into queue */
            sched->queue->ops->push(sched->queue, next);
            break;;
        }
    } while(next->state != RUNNING);
    
    sched->ctx.running = next;
    current_running = next;
    load_page_directory(sched->ctx.running->page_dir);

    return 0;
}

/* Default round robin scheduler behavior */
static int sched_default(struct scheduler* sched)
{
    dbgprintf("%x\n", sched);
    assert(sched->queue->_list != NULL);
    SCHED_VALIDATE(sched);
    if (sched->ctx.running == NULL){
        sched->ctx.running = sched->queue->ops->pop(sched->queue);
        current_running = sched->ctx.running;
        dbgprintf("peeking %s\n", sched->ctx.running->name);
    }
    
    sched->ctx.running->yields++;

    CRITICAL_SECTION({
        struct pcb* current = sched->ctx.running;
        
        if(current_running->state != PCB_NEW) pcb_save_context(current_running);
        dbgprintf("Saving context of %s (%x)\n", current_running->name, sched);
        
        assert(sched->queue->_list != NULL);
        /* Switch to next PCB, should be chosen by flag? */
        PANIC_ON_ERR(sched_round_robin(sched));

        assert(0);

        pcb_restore_context(sched->ctx.running);
        dbgprintf("Switching too PCB %s with page dir: %x, stack: %x, kstack: %x\n", sched->ctx.running->name, sched->ctx.running->page_dir, sched->ctx.running->ctx.esp, sched->ctx.running->kesp);
    });

    return 0;
}

/**
 * @brief Cleans up the given pcb
 * 
 * @param sched The scheduler to clean up on
 * @return int 0 on success, error code on failure
 */
static int sched_exit(struct scheduler* sched)
{
    SCHED_VALIDATE(sched);

    sched->exits++;  

    /* Add cleanup routine to work queue */
    RETURN_ON_ERR(work_queue_add(&pcb_cleanup_routine, (void*)sched->ctx.running->pid, NULL));
    sched->ctx.running = NULL;

    /* Switch to next PCB, dont need to store context */
    (void)sched_round_robin(sched);

    pcb_restore_context(sched->ctx.running);

    return 0;
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
    
    return 0;
}

/* Kernel scheduling API */

void kernel_sleep(int time)
{
    get_default_scheduler()->ops->sleep(get_default_scheduler(), time);
}

void kernel_yield()
{   
    dbgprintf("Yielding...\n");
    assert(get_default_scheduler()->ops->schedule(get_default_scheduler()) == 0);
}

void kernel_exit()
{
    get_default_scheduler()->ops->exit(get_default_scheduler());

    UNREACHABLE();
}

void block()
{
    struct pcb* current = get_default_scheduler()->ops->consume(get_default_scheduler());
    get_default_scheduler()->ops->block(get_default_scheduler(), current);
}

void unblock(int pid)
{
    //pcb_set_running(pid);
}



/* This code is resposible for choosing the next pcb to run */
void context_switch_process()
{
    ASSERT_CRITICAL();

    pcb_save_context(current_running);   

    current_running = current_running->next;
    
    /* quick hacky fix*/
    if(current_running == NULL){
        current_running = pcb_get_new_running();
    }

    while(current_running->state != RUNNING){

        switch (current_running->state){
        case ZOMBIE:{
                struct pcb* old = current_running;
                current_running = current_running->next;
                old->state = CLEANING;

                work_queue_add(&pcb_cleanup_routine, (void*)old->pid, NULL);
                dbgprintf("Adding work to clean up PID %d\n", old->pid);
            }
            break;
        case PCB_NEW:{
                dbgprintf("Entering scheduler... NEW\n");
                dbgprintf("Running new PCB %s (PID %d) with page dir: %x: stack: %x kstack: %x\n", current_running->name, current_running->pid, current_running->page_dir, current_running->ctx.esp, current_running->kesp);
                load_page_directory(current_running->page_dir);
                //pcb_dbg_print(current_running);

                if(current_running->is_process){
                    tss.esp_0 = (uint32_t)current_running->kebp;
                    tss.ss_0 = GDT_KERNEL_DS;
                }

                //load_data_segments(GDT_KERNEL_DS);
                start_pcb(current_running);
                return; /* not sure if it should return or break */
            }
            break; /* Never reached. */
        case SLEEPING:{
                if(timer_get_tick() >= current_running->sleep)
                current_running->state = RUNNING;
                else
                    current_running = current_running->next;
                }
            break;
        default:
            current_running = current_running->next;
            break;
        }
    
        /* quick hacky fix*/
        if(current_running == NULL){
            current_running = pcb_get_new_running();
        }
    }
    load_page_directory(current_running->page_dir);
    //dbgprintf("Switching too PCB %s with page dir: %x, stack: %x, kstack: %x\n", current_running->name, current_running->page_dir, current_running->ctx.esp, current_running->kesp);
    pcb_restore_context(current_running);
}
