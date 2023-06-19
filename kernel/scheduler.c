#include <scheduler.h>
#include <memory.h>
#include <pcb.h>
#include <timer.h>
#include <serial.h>
#include <assert.h>
#include <work.h>

#include <arch/gdt.h>
#include <arch/tss.h>

struct scheduler;
struct scheduler_ops;
/* exposed operator functions */
static int sched_prioritize(struct scheduler* sched, struct pcb* pcb);
static int sched_default(struct scheduler* sched);
static int sched_add(struct scheduler* sched, struct pcb* pcb);
static int sched_sleep(struct scheduler* sched, int time);

typedef enum scheduler_flags {
    UNUSED = 1 << 0,
} sched_flag_t;

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
    struct pcb_queue* queue;
    struct pcb_queue* priority;

    struct {
        struct pcb* running;
    } ctx;
};

/* Default scheduler operations */
static struct scheduler_ops scheduler_default_ops = {
    .prioritize = &sched_prioritize,
    .add = &sched_prioritize,
    .schedule = &sched_default,
    .sleep = &sched_sleep
};


struct pcb* sched_ctx_get_running()
{
    /* This function should return the current running process from the scheduler context. */
    NOT_IMPLEMENTED();
    return NULL;
}

static int sched_sleep(struct scheduler* sched, int time)
{
    sched->ctx.running->sleep = timer_get_tick() + time;
    sched->ctx.running->state = SLEEPING;
    sched->ops->schedule(sched);
}

static int sched_prioritize(struct scheduler* sched, struct pcb* pcb)
{
    NOT_IMPLEMENTED();
    return -1;
}

static int sched_default(struct scheduler* sched)
{
    NOT_IMPLEMENTED();
    return -1;
}

static int sched_add(struct scheduler* sched, struct pcb* pcb)
{
    NOT_IMPLEMENTED();
    return -1;
}

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

    current_running = current_running->next;
    assert(current_running != NULL);
    while(current_running->state != RUNNING){
        switch (current_running->state){
        case STOPPED:
            current_running = current_running->next;
            break;
        case ZOMBIE:
            ;
            struct pcb* old = current_running;
            current_running = current_running->next;
            old->state = CLEANING;

            work_queue_add(&pcb_cleanup_routine, (void*)old->pid, NULL);
            dbgprintf("Adding work to clean up PID %d\n", old->pid);
            break;
        case PCB_NEW:
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
            break; /* Never reached. */
        case SLEEPING:
            if(timer_get_tick() >= current_running->sleep)
                current_running->state = RUNNING;
            else
                current_running = current_running->next;
            break;
        default:
            current_running = current_running->next;
            break;
        }
    }
    load_page_directory(current_running->page_dir);
    //dbgprintf("[Context Switch] Switching too PCB %s with page dir: %x, stack: %x, kstack: %x\n", current_running->name, current_running->page_dir, current_running->ctx.esp, current_running->kesp);
}
