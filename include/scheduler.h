#ifndef __SCHEDULER_H
#define __SCHEDULER_H

void kernel_sleep(int time);
void kernel_yield();
void kernel_exit();
void block();
void unblock(int pid);
void context_switch_process();

#define WAIT(pred) while(pred){kernel_yield();}

void sched_save_ctx();
void sched_restore_ctx();

void pcb_restore_ctx();
void pcb_save_ctx();

#endif