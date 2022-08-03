#ifndef __SCHEDULER_H
#define __SCHEDULER_H

void sleep(int time);
void yield();
void exit();
void block();
void unblock(int pid);
void context_switch();

#endif