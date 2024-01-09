#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <pcb.h>

void kernel_sleep(int time);
void kernel_yield();
void kernel_exit();
void block();
void unblock(int pid);
void context_switch_process();

#define WAIT(pred) while(pred){kernel_yield();}

/* Checks that the given scheduler is initiated. */
#define SCHED_VALIDATE(s) if(!((s)->flags & SCHED_INITIATED)) { return -ERROR_SCHED_INVALID;}

struct scheduler;
struct scheduler_ops;

/* Scheduler flags */
typedef enum scheduler_flags {
    SCHED_UNUSED = 1 << 0,
    SCHED_INITIATED = 1 << 1
} sched_flag_t;

/* Scheduler operations */
struct scheduler_ops {
    error_t (*prioritize)(struct scheduler* sched, struct pcb* pcb);
    error_t (*schedule)(struct scheduler* sched);
    error_t (*add)(struct scheduler* sched, struct pcb* pcb);
    error_t (*block)(struct scheduler* sched, struct pcb* pcb);
    error_t (*sleep)(struct scheduler* sched, int time);
    error_t (*exit)(struct scheduler* sched);
    error_t (*yield)(struct scheduler* sched);
    struct pcb* (*consume)(struct scheduler* sched);
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


error_t sched_init_default(struct scheduler* sched, sched_flag_t flags);

/* asm functions */
void pcb_restore_ctx();
void pcb_save_ctx();

/* Temporary */
struct scheduler* get_scheduler();

#endif