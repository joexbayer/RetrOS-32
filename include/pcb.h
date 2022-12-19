#ifndef PCB_H
#define PCB_H

#include <util.h>
#include <windowmanager.h>

#define MAX_NUM_OF_PCBS 10
#define pcb_max_name_length 25

/* TODO: Move to new file */
enum {
    STOPPED,
    RUNNING,
    NEW,
    BLOCKED,
    SLEEPING,
    ZOMBIE
};

/* Should be in list.h? */
enum {
    SINGLE_LINKED,
    DOUBLE_LINKED
};

struct pcb {
      uint32_t ebp;
      uint32_t esp;
      void (*eip)();
      uint32_t k_esp;
      uint32_t k_ebp;
      uint32_t is_process;
      uint32_t fpu[32];
      /* DO NOT NOT CHANGE ABOVE.*/
      uint8_t running;
      int16_t pid;
      uint16_t sleep_time;
      uint32_t org_stack;

      uint32_t blocked_count;

      uint32_t* page_dir;

      char name[pcb_max_name_length];

      struct window* window;

      struct pcb *next;
      struct pcb *prev;
}__attribute__((packed));


extern struct pcb* current_running;

void init_pcbs();
void start_tasks();
void start_pcb();
int add_pcb( void (*entry)(), char* name);
void print_pcb_status();
int create_process(char* program);

void pcb_set_running(int pid);
void pcb_set_blocked(int pid);

void pcb_queue_push_running(struct pcb* pcb);
void pcb_queue_remove(struct pcb* pcb);
void pcb_queue_push(struct pcb** queue, struct pcb* pcb, int type);
struct pcb* pcb_queue_pop(struct pcb** queue, int type);
void pcb_print_queues();

int pcb_cleanup(int pid);

struct pcb* pcb_get_new_running();

void pcb_memory_usage();


/* functions in entry.s */
void _start_pcb();
void _context_switch();


#endif