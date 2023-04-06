#ifndef PCB_H
#define PCB_H

struct pcb;

#include <util.h>
#include <sync.h>
#include <gfx/window.h>
#include <memory.h>
#include <fs/inode.h>

#define MAX_NUM_OF_PCBS 64
#define pcb_max_name_length 25

/* TODO: Move to new file */
enum pcb_states {
    STOPPED,
    RUNNING,
    PCB_NEW,
    BLOCKED,
    SLEEPING,
    ZOMBIE,
    CLEANING
};

struct pcb {
    uint32_t ebp;
    uint32_t esp;
    void (*eip)();
    uint32_t kesp;
    uint32_t kebp;
    uint32_t is_process;
    uint32_t fpu[32];
    int args;
    char** argv;
    /* DO NOT NOT CHANGE ABOVE.*/
    char name[pcb_max_name_length];
    uint8_t state;
    int16_t pid;
    uint16_t sleep;
    uint32_t stack_ptr;
    uint32_t* page_dir;
    uint32_t data_size;

    /* stats */
    int kallocs;
    int yields;
    uint32_t blocked_count;

    struct gfx_window* gfx_window;
    struct terminal* term;

    inode_t current_directory;

    struct allocation* allocations;
    int used_memory;

    struct pcb* parent;
    struct pcb *next;
    struct pcb *prev;
}__attribute__((__packed__));

extern struct pcb* current_running;

/* Forward declaration */
struct pcb_queue;

/**
 * @brief This defines a set of operations for a PCB queue.
 * The pcb_queue_operations structure defines a set of operations that can be performed
 * on a PCB queue. These operations include pushing and removing PCBs from the queue,
 * as well as getting the next PCB to be executed.
 */
struct pcb_queue_operations {
	void (*push)(struct pcb_queue* queue, struct pcb* pcb);
	void (*add)(struct pcb_queue* queue, struct pcb* pcb);
	void (*remove)(struct pcb_queue* queue, struct pcb* pcb);
	struct pcb* (*pop)(struct pcb_queue* queue);
};

/**
 * @brief This defines a PCB queue.
 * The pcb_queue structure defines a PCB queue, which contains a set of operations defined
 * by the pcb_queue_operations structure. It also contains a pointer to the head of the queue,
 * a spinlock to protect access to the queue, and a count of the total number of PCBs in the queue.
 */
struct pcb_queue {
	struct pcb_queue_operations* ops;
	struct pcb* _list;
	spinlock_t spinlock;
	int total;
};

void pcb_queue_attach_ops(struct pcb_queue* q);

void init_pcbs();
void pcb_start();
void start_pcb();
int pcb_create_kthread( void (*entry)(), char* name);
int pcb_create_process(char* program, int args, char** argv);

void pcb_set_running(int pid);

void pcb_dbg_print(struct pcb* pcb);

void pcb_queue_remove_running(struct pcb* pcb);
void pcb_queue_push_running(struct pcb* pcb);

int pcb_cleanup_routine(int pid);

struct pcb* pcb_get_new_running();
struct pcb_queue* pcb_new_queue();

/* functions in entry.s */
void _start_pcb();
void _context_switch();

void idletask();
void dummytask();
void Genesis();


#endif