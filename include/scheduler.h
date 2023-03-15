#ifndef __SCHEDULER_H
#define __SCHEDULER_H

void kernel_sleep(int time);
void kernel_yield();
void kernel_exit();
void kernel_block();
void kernel_unblock(int pid);
void context_switch();

#endif