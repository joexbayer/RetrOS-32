/**
 * @file pcb.c
 * @author Joe Bayer (joexbayer)
 * @brief Manages PCBs creation, destruction and their status.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <pcb.h>
#include <serial.h>
#include <memory.h>
#include <scheduler.h>
#include <fs/fs.h>
#include <assert.h>

#include <gfx/gfxlib.h>

#define STACK_SIZE 0x2000
static const char* pcb_status[] = {"stopped ", "running ", "new     ", "blocked ", "sleeping", "zombie"};

struct pcb_queue;

struct pcb_queue_operations {
	void (*push)(struct pcb_queue* queue, struct pcb* pcb);
	void (*add)(struct pcb_queue* queue, struct pcb* pcb);
	void (*remove)(struct pcb_queue* queue, struct pcb* pcb);
	struct pcb* (*pop)(struct pcb_queue* queue);
} pcb_queue_default_ops;
/* TODO: add default ops */

struct pcb_queue {
	struct pcb_queue_operations* ops;
	struct pcb* list;
	int total;
};


static struct pcb pcbs[MAX_NUM_OF_PCBS];
static int pcb_count = 0;
static struct pcb* pcb_running_queue = &pcbs[0];
static struct pcb* pcb_blocked_queue = NULL;

/**
 * Current running PCB, used for context aware
 * functions such as windows drawing to the screen.
 */
struct pcb* current_running = &pcbs[0];

/**
 * @brief Push pcb struct at the end of given queue
 * 
 * @param queue 
 * @param pcb 
 */
void pcb_queue_push(struct pcb** queue, struct pcb* pcb)
{
	assert(*queue != NULL);
	CLI();
	struct pcb* current = (*queue);
	if(current == NULL)
	{
		(*queue) = pcb;
		return;
	}
	while (current->next == NULL)
		current = current->next;
	current->next = pcb;
	pcb->next = NULL;
	STI();
	dbgprintf("[SINGLE QUEUE] Added %s to a queue\n", pcb->name);
}

void pcb_queue_add(struct pcb** queue, struct pcb* pcb)
{
	CRITICAL_SECTION({

		struct pcb* prev = (*queue)->prev;
		(*queue)->prev = pcb;
		pcb->next = (*queue);
		prev->next = pcb;
		pcb->prev = prev;

	});
}

void pcb_queue_insert_after(struct pcb *node, struct pcb *new_node) {
	
    new_node->prev = node;
    new_node->next = node->next;
    if (node->next != NULL) {
        node->next->prev = new_node;
    }
    node->next = new_node;
}

void pcb_queue_remove(struct pcb* pcb)
{
	CRITICAL_SECTION({

		struct pcb* prev = pcb->prev;
		prev->next = pcb->next;
		pcb->next->prev = prev;

		pcb->next = NULL;
		pcb->prev = NULL;
		
	});
}

struct pcb* pcb_queue_pop(struct pcb **head) {
    if (*head == NULL) {
        return NULL;
    }
    struct pcb *front = *head;
    *head = front->next;
    if (*head != NULL) {
        (*head)->prev = NULL;
    }
    front->next = NULL;
    front->prev = NULL;
    return front;
}

/**
 * @brief Wrapper function to push to running queue
 * 
 * @param pcb 
 */
void pcb_queue_push_running(struct pcb* pcb)
{
	dbgprintf("[QUEUE] Added %s to the running queue\n", pcb->name);
	pcb_queue_add(&pcb_running_queue, pcb);
}

struct pcb* pcb_get_new_running()
{
	assert(pcb_running_queue != NULL);
	return pcb_running_queue;
}

void Genesis()
{
	dbgprintf("[GEN] Genesis running!\n");
	struct gfx_window* window = gfx_new_window(400, 100);
	while(1)
	{
		__gfx_draw_rectangle(0, 0, 400, 100, VESA8_COLOR_LIGHT_GRAY5);
		__gfx_draw_rectangle(3, 3, 395, 66, VESA8_COLOR_LIGHT_GRAY1);

		gfx_line(2, 2, 66, GFX_LINE_OUTER_VERTICAL, VESA8_COLOR_BLUE);
		gfx_line(3, 2, 396, GFX_LINE_INNER_HORIZONTAL, VESA8_COLOR_BLUE);

		gfx_line(396, 2, 66, GFX_LINE_INNER_VERTICAL, VESA8_COLOR_BLUE);
		gfx_line(2, 68, 396, GFX_LINE_OUTER_HORIZONTAL, VESA8_COLOR_BLUE);

		print_pcb_status();

		__gfx_draw_format_text(10, 80, VESA8_COLOR_BLACK, "Total: %d", pcb_count);
		
		gfx_commit();
		sleep(200);
	}
}

/**
 * @brief Main function of the PCB background process.
 * Printing out information about the currently running processes.
 */
void print_pcb_status()
{
	int done_list[MAX_NUM_OF_PCBS];
	int done_list_count = 0;
	
	__gfx_draw_text(10, 7, "Name           Status    Memory    Stack     PID", VESA8_COLOR_LIGHT_BROWN);
	gfx_line(10, 7+9, 382, GFX_LINE_HORIZONTAL, VESA8_COLOR_GRAY2);
	for (int i = 0; i < MAX_NUM_OF_PCBS; i++)
	{
		if(pcbs[i].pid == -1)
			continue;

		int largest = 0;
		uint32_t largest_amount = 0;
		for (int j = 0; j < MAX_NUM_OF_PCBS; j++)
		{
			if(pcbs[j].pid == -1)
				continue;
			
			int found = 0;
			for (int k = 0; k < done_list_count; k++)
			{
				if(done_list[k] == j){
					found = 1;
					break;
				}
			}

			if(found)
				continue;;
			

			if(pcbs[j].ebp-pcbs[j].esp >= largest_amount){
				largest_amount = pcbs[j].ebp-pcbs[j].esp;
				largest = j;
			}
		}

		done_list[done_list_count] = largest;
		done_list_count++;
		//__gfx_draw_format_text(10, 10+done_list_count*8, VESA8_COLOR_BLACK, " %d  0x%x  %s  %s  %s\n", pcbs[largest].pid, pcbs[largest].used_memory, status[pcbs[largest].running], pcbs[largest].is_process == 1 ? "Process" : "kthread", pcbs[largest].name);

		__gfx_draw_format_text(10, 10+done_list_count*8, VESA8_COLOR_BLACK, "%s", pcbs[largest].name);
		__gfx_draw_format_text(10 + 15*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%s", pcb_status[pcbs[largest].running]);
		__gfx_draw_format_text(10+15*8+10*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%d", pcbs[largest].used_memory);
		__gfx_draw_format_text(10+15*8+10*8 + 10*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "0x%x", pcbs[largest].esp);
		__gfx_draw_format_text(10+15*8+10*8+10*8+11*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%d", pcbs[largest].pid);
	}
}

void pcb_set_blocked(int pid)
{
	if(pid < 0 || pid > MAX_NUM_OF_PCBS)
		return;

	pcbs[pid].running = BLOCKED;

	pcb_queue_remove(&pcbs[pid]);
	pcb_queue_push(&pcb_blocked_queue, &pcbs[pid]);

	pcbs[pid].blocked_count++;
}

void pcb_set_running(int pid)
{
	if(pid < 0 || pid > MAX_NUM_OF_PCBS)
		return;

	pcbs[pid].running = RUNNING;
}

/**
 * @brief Sets the process with given pid to stopped. Also frees the process's stack.
 * 
 * @param pid id of the process.
 * @return int index of pcb, -1 on error.
 */
int pcb_cleanup_routine(int pid)
{
	ASSERT_CRITICAL();

	if(pid < 0 || pid > MAX_NUM_OF_PCBS)
		return -1;

	dbgprintf("[PCB] Cleaning zombie process %s\n", pcbs[pid].name);

	if(pcbs[pid].gfx_window != NULL){
		gfx_destory_window(pcbs[pid].gfx_window);
	}

	pcb_queue_remove(&pcbs[pid]);
	dbgprintf("[PCB] Cleanup on PID %d stack: 0x%x (original: 0x%x)\n", pid, pcbs[pid].esp, pcbs[pid].stack_ptr);
	
	pcb_count--;
	
	if(pcbs[pid].is_process){
		vmem_cleanup_process(&pcbs[pid]);
	}
	kfree((void*)pcbs[pid].stack_ptr);

	//memset(&pcbs[pid], 0, sizeof(struct pcb));

	pcbs[pid].running = STOPPED;
	pcbs[pid].pid = -1;

	return pid;
}

/**
 * @brief Initializes the PCBs struct members and importantly allocates the stack.
 * 
 * @param pid id of process
 * @param pcb pointer to pcb
 * @param entry pointer to entry function
 * @param name name of process.
 * @return int 1 on success -1 on error.
 */
int pcb_init_kthread(int pid, struct pcb* pcb, void (*entry)(), char* name)
{
	uint32_t stack = (uint32_t) kalloc(STACK_SIZE);
	memset((void*)stack, 0, STACK_SIZE);
	if((void*)stack == NULL)
	{
		dbgprintf("[PCB] STACK == NULL");
		return -1;
	}

	/* Stack grows down so we want the upper part of allocated memory.*/ 
	pcb->ebp = stack+STACK_SIZE-1;
	pcb->esp = stack+STACK_SIZE-1;
	pcb->kesp = pcb->esp;
	pcb->kebp = pcb->kesp;
	pcb->eip = entry;
	pcb->running = NEW;
	pcb->pid = pid;
	pcb->stack_ptr = stack;
	pcb->allocations = NULL;
	pcb->used_memory = 0;
	pcb->kallocs = 0;
	pcb->page_dir = kernel_page_dir;
	pcb->is_process = 0;

	memcpy(pcb->name, name, strlen(name)+1);

	dbgprintf("[INIT KTHREAD] Created new kernel thread!\n");

	return 1;
}

int pcb_create_process(char* program)
{
	CLI();
	/* Load process from disk */
	inode_t inode = fs_open(program);
	if(inode == 0)
		return 0;

	char buf[MAX_FILE_SIZE];
	int read = fs_read(inode, (char* )buf, MAX_FILE_SIZE);
	fs_close(inode);
	dbgprintf("[INIT PROCESS] Reading %s from disk (%d bytes)\n", program, read);

	/* Create stack and pcb */
	 int i; /* Find a pcb is that is "free" */
	for(i = 0; i < MAX_NUM_OF_PCBS; i++)
		if(pcbs[i].running == STOPPED)
			break;
		
	assert(pcbs[i].running == STOPPED);
	
	struct pcb* pcb = &pcbs[i];

	char* pname = "program";

	pcb->eip = (void (*)()) 0x1000000; /* External programs start */
	pcb->pid = i;
	pcb->data_size = read;
	memcpy(pcb->name, pname, strlen(pname)+1);
	pcb->esp = 0xEFFFFFF0;
	pcb->ebp = pcb->esp;
	pcb->stack_ptr = (uint32_t) kalloc(STACK_SIZE);
	memset((void*)pcb->stack_ptr, 0, STACK_SIZE);
	pcb->kesp = pcb->stack_ptr+STACK_SIZE-1;
	dbgprintf("[INIT PROCESS] Setup PCB %d for %s\n", i, program);
	pcb->kebp = pcb->kesp;
	pcb->term = current_running->term;
	pcb->is_process = 1;
	pcb->kallocs = 0;

	/* Memory map data */
	vmem_init_process(pcb, buf, read);

	pcb_queue_push_running(pcb);

	pcb_count++;
	STI();
	dbgprintf("[INIT PROCESS] Created new process!\n");
	pcb->running = NEW;
	/* Run */
	return i;
}

/**
 * @brief Add a pcb to the list of running proceses.
 * Also instantiates the PCB itself.
 * 
 * @param entry pointer to entry function.
 * @param name name of process
 * @return int amount of running processes, -1 on error.
 */
int pcb_create_kthread(void (*entry)(), char* name)
{   
	CLI();
	if(MAX_NUM_OF_PCBS == pcb_count)
		return -1;

	int i; /* Find a pcb is that is "free" */
	for(i = 0; i < MAX_NUM_OF_PCBS; i++)
		if(pcbs[i].running == STOPPED)
			break;
	
	int ret = pcb_init_kthread(i, &pcbs[i], entry, name);
	if(!ret){
		STI();
		return ret;
	}

	pcb_queue_push_running(&pcbs[i]);

	pcb_count++;
	dbgprintf("Added %s\n", name);
	STI();
	return i;
}

void start_pcb()
{   
	current_running->running = RUNNING;
	dbgprintf("[START PCB] Starting pcb!\n");
	_start_pcb(); /* asm function */
}
/**
 * @brief Sets all PCBs state to stopped. Meaning they can be started.
 * Also starts the PCB background process.
 */
void pcb_init()
{   

	/* Stopped processes are eligible to be "replaced." */
	for (int i = 0; i < MAX_NUM_OF_PCBS; i++)
	{
		pcbs[i].running = STOPPED;
		pcbs[i].pid = -1;
	}
	current_running = &pcbs[0];
	pcbs[0].next = &pcbs[0];
	pcbs[0].prev = &pcbs[0];

	int ret = pcb_create_kthread(&Genesis, "Genesis");
	if(ret < 0) return; // error

	dbgprintf("[PCB] All process control blocks are ready.\n");

}

void pcb_start()
{
	start_pcb();
	/* We will never reach this.*/
}
