#ifndef __SCHEDULER_H
#define __SCHEDULER_H

void kernel_sleep(int time);
void kernel_yield();
void kernel_exit();
void block();
void unblock(int pid);
void context_switch();

#define WAIT(pred) while(pred){kernel_yield();}

#endif