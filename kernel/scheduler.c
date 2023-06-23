#include <scheduler.h>
#include <memory.h>
#include <pcb.h>
#include <timer.h>
#include <serial.h>
#include <assert.h>
#include <work.h>

#include <arch/gdt.h>
#include <arch/tss.h>

/* Checks that the given scheduler is initiated. */
#define SCHED_VALIDATE(sched) if(!(sched->flags & SCHED_INITIATED)) {return -ERROR_SCHED_INVALID;}

struct scheduler;
struct scheduler_ops;

/* exposed operator functions */
static int sched_prioritize(struct scheduler* sched, struct pcb* pcb);
static int sched_default(struct scheduler* sched);
static int sched_add(struct scheduler* sched, struct pcb* pcb);
static int sched_sleep(struct scheduler* sched, int time);

/* Scheduler flags */
typedef enum scheduler_flags {
    SCHED_UNUSED = 1 << 0,
    SCHED_INITIATED = 1 << 1
} sched_flag_t;

/* Scheduler operations */
struct scheduler_ops {
    int (*prioritize)(struct scheduler* sched, struct pcb* pcb);
    int (*schedule)(struct scheduler* sched);
    int (*add)(struct scheduler* sched, struct pcb* pcb);
    int (*sleep)(struct scheduler* sched, int time);
};

struct scheduler {
    unsigned char flags;
    unsigned int yields;
    unsigned int exits;

    struct scheduler_ops* ops;
    /* queue is the list of ready PCBs */
    struct pcb_queue* queue;
    struct pcb_queue* priority;

    struct {
        struct pcb* running;
    } ctx;
};

/* Default scheduler operations */
static struct scheduler_ops sched_default_ops = {
    .prioritize = &sched_prioritize,
    .add = &sched_prioritize,
    .schedule = &sched_default,
    .sleep = &sched_sleep
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
    if(sched->queue){
        return -ERROR_PCB_QUEUE_CREATE;
    }

    sched->flags = flags | SCHED_INITIATED;

    return 0;
}

static int sched_sleep(struct scheduler* sched, int time)
{
    SCHED_VALIDATE(sched);

    sched->ctx.running->sleep = timer_get_tick() + time;
    sched->ctx.running->state = SLEEPING;

    sched->ops->schedule(sched);

    return 0;
}

static int sched_prioritize(struct scheduler* sched, struct pcb* pcb)
{
    SCHED_VALIDATE(sched);

    NOT_IMPLEMENTED();
    return -1;
}

static int sched_round_robin(struct scheduler* sched)
{
    struct pcb* next;
    SCHED_VALIDATE(sched);
    ASSERT_CRITICAL();

    /* Add current running context to queue */
    if(sched->ctx.running != NULL){
        sched->queue->ops->push(sched->queue, sched->ctx.running);
        sched->ctx.running = NULL;
    }

    /* get next pcb from queue */
    do {
        /* If queue is empty, return error */
        next = sched->queue->ops->pop(sched->queue); 
        if(next == NULL){
            return -ERROR_PCB_QUEUE_EMPTY;
        }

        /* If next is sleeping, check if it should be woken up */
        switch (next->state){
        case SLEEPING:
            if(next->sleep < timer_get_tick()){
                next->state = RUNNING;
                break;
            }
        case RUNNING:
            break;
        default:
            /* This should be rare, only when blocked threads are in this queue (blocked threads without synchronizational needs)*/
            sched->queue->ops->push(sched->queue, next);
            break;
        }
    } while(next->state != RUNNING);
    
    sched->ctx.running = next;

    return 0;
}

/* Default round robin scheduler behavior */
static int sched_default(struct scheduler* sched)
{
    SCHED_VALIDATE(sched);

    sched->ctx.running->yields++;

    CRITICAL_SECTION({
        pcb_save_context();

        /* Switch to next PCB, should be chosen by flag? */
        sched_round_robin(sched);

        pcb_restore_context();
    });

    return 0;
}

static int sched_add(struct scheduler* sched, struct pcb* pcb)
{
    SCHED_VALIDATE(sched);

    RETURN_ON_ERR(sched->queue->ops->push(sched->queue, pcb));
    
    return 0;
}

/* Kernel scheduling API */

void kernel_sleep(int time)
{
    current_running->sleep = timer_get_tick() + time;
    current_running->state = SLEEPING;
    kernel_yield();
}

void kernel_yield()
{   
    current_running->yields++;
    //dbgprintf("%s called yield\n", current_running->name);
    context_switch_entry();
}

void kernel_exit()
{
    dbgprintf("%s (PID %d) called Exit\n", current_running->name, current_running->pid);
    current_running->state = ZOMBIE;
    context_switch_entry();

    UNREACHABLE();
}

void block()
{
    current_running->state = BLOCKED;
    current_running->blocked_count++;
    context_switch_entry();
}

void unblock(int pid)
{
    pcb_set_running(pid);
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
                dbgprintf("[Context Switch] Running new PCB %s (PID %d) with page dir: %x: stack: %x kstack: %x\n", current_running->name, current_running->pid, current_running->page_dir, current_running->ctx.esp, current_running->kesp);
                load_page_directory(current_running->page_dir);
                //pcb_dbg_print(current_running);

                if(current_running->is_process){
                    tss.esp_0 = (uint32_t)current_running->kebp;
                    tss.ss_0 = GDT_KERNEL_DS;
                }

                //load_data_segments(GDT_KERNEL_DS);
                start_pcb();
                UNREACHABLE();
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
    // dbgprintf("[Context Switch] Switching too PCB %s with page dir: %x, stack: %x, kstack: %x\n", current_running->name, current_running->page_dir, current_running->ctx.esp, current_running->kesp);
    pcb_restore_context(current_running);
}
