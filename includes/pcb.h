#ifndef PCB_H
#define PCB_H

#include <util.h>

enum {
    RUNNING = 1
};

struct pcb {
      uint32_t ebp;
      uint32_t esp;
      uint32_t eip;
      uint32_t fpu[32];
      /* DO NOT NOT CHANGE ABOVE.*/
      uint8_t running;
      uint16_t pid;

      struct pcb *next;
}__attribute__((packed));


extern struct pcb* current_running;
void context_switch();
void init_pcbs();
void start_tasks();
int add_pcb(uint32_t entry);
void yield();

/* functions in entry.s */
void _start_pcb();
void _context_switch();

#endif