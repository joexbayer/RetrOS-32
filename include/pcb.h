#ifndef PCB_H
#define PCB_H

#include <util.h>

#define pcb_max_name_length 25

/* TODO: Move to new file */
enum {
    STOPPED,
    RUNNING,
    NEW,
    BLOCKED,
    SLEEPING
};

struct pcb {
      uint32_t ebp;
      uint32_t esp;
      void (*eip)();
      uint32_t fpu[32];
      /* DO NOT NOT CHANGE ABOVE.*/
      uint8_t running;
      int16_t pid;
      uint16_t sleep_time;
      uint32_t org_stack;

      char name[pcb_max_name_length];

      struct pcb *next;
      struct pcb *prev;
}__attribute__((packed));


extern struct pcb* current_running;
void context_switch();
void init_pcbs();
void start_tasks();
int stop_task(int pid);
int add_pcb( void (*entry)(), char* name);
void print_pcb_status();

void yield();
void sleep(int time);
void block();
void unblock(int pid);
void exit();

/* functions in entry.s */
void _start_pcb();
void _context_switch();


#endif