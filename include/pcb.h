#ifndef PCB_H
#define PCB_H

struct pcb;

#include <util.h>
#include <gfx/window.h>
#include <memory.h>

#define MAX_NUM_OF_PCBS 24
#define pcb_max_name_length 25

/* TODO: Move to new file */
enum pcb_states {
    STOPPED,
    RUNNING,
    NEW,
    BLOCKED,
    SLEEPING,
    ZOMBIE
};

struct pcb {
    uint32_t ebp;
    uint32_t esp;
    void (*eip)();
    uint32_t kesp;
    uint32_t kebp;
    uint32_t is_process;
    uint32_t fpu[32];
    /* DO NOT NOT CHANGE ABOVE.*/
    uint8_t running;
    int16_t pid;
    uint16_t sleep;
    uint32_t stack_ptr;

    uint32_t blocked_count;

    uint32_t* page_dir;
    uint32_t data_size;

    /* stats */
    int kallocs;

    char name[pcb_max_name_length];

    struct gfx_window* gfx_window;
    struct terminal* term;

    struct allocation* allocations;
    int used_memory;

    struct pcb *next;
    struct pcb *prev;
}__attribute__((packed));

extern struct pcb* current_running;

void pcb_init();
void pcb_start();
void start_pcb();
int pcb_create_kthread( void (*entry)(), char* name);
void print_pcb_status();
int pcb_create_process(char* program);

void pcb_set_running(int pid);
void pcb_set_blocked(int pid);

void pcb_queue_push_running(struct pcb* pcb);
void pcb_queue_remove(struct pcb* pcb);
void pcb_queue_push(struct pcb** queue, struct pcb* pcb);
struct pcb* pcb_queue_pop(struct pcb **head);

int pcb_cleanup_routine(int pid);

struct pcb* pcb_get_new_running();

/* functions in entry.s */
void _start_pcb();
void _context_switch();


#endif