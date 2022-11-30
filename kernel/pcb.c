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
#include <sync.h>
#include <timer.h>
#include <net/netdev.h>
#include <scheduler.h>
#include <terminal.h>
#include <fs/fs.h>

#include <windowmanager.h>

#define stack_size 0x2000

static const char* status[] = {"stopped ", "running ", "new     ", "blocked ", "sleeping"};

static struct pcb pcbs[MAX_NUM_OF_PCBS];
static int pcb_count = 0;

struct pcb* current_running = &pcbs[0];
static struct pcb* pcb_running_queue = &pcbs[0];
static struct pcb* pcb_blocked_queue = NULL;

/**
 * @brief Push pcb struct at the end of given queue
 * 
 * @param queue 
 * @param pcb 
 */
void pcb_queue_push(struct pcb** queue, struct pcb* pcb, int type)
{
	CLI();
	switch (type)
	{
	case SINGLE_LINKED:
		;
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
		dbgprintf("[SINGLE QUEUE] Added %s to a queue\n", pcb->name);
		break;
	
	case DOUBLE_LINKED:
		;
		struct pcb* prev = (*queue)->prev;
		(*queue)->prev = pcb;
		pcb->next = (*queue);
		prev->next = pcb;
		pcb->prev = prev;

		dbgprintf("[DOUBLE QUEUE] Added %s to a queue\n", pcb->name);

	default:
		break;
	}

	STI();
}

struct pcb* pcb_get_new_running()
{
	return pcb_running_queue;
}


void pcb_queue_remove(struct pcb* pcb)
{
	CLI();
	struct pcb* prev = pcb->prev;
	prev->next = pcb->next;
	pcb->next->prev = prev;

	pcb->next = NULL;
	pcb->prev = NULL;
	STI();

	dbgprintf("[QUEUE] Removed %s to a queue\n", pcb->name);
}

struct pcb* pcb_queue_pop(struct pcb** queue, int type)
{
	if((*queue)->pid == 0)
		return NULL;

	CLI();

	switch (type)
	{
	case SINGLE_LINKED:
		struct pcb* current = (*queue);

		if(current == NULL)
			return NULL;

		(*queue) = (*queue)->next;
		STI();
		dbgprintf("[SINGLE QUEUE] Poped %s to a queue\n", current->name);
		return current;
	
	case DOUBLE_LINKED:
		/* code */
		break;

	default:
		break;
	}

	STI();

	return NULL;
}



/**
 * @brief Wrapper function to push to running queue
 * 
 * @param pcb 
 */
void pcb_queue_push_running(struct pcb* pcb)
{
	dbgprintf("[QUEUE] Added %s to a running queue\n", pcb->name);
	pcb_queue_push(&pcb_running_queue, pcb, DOUBLE_LINKED);
}

void gensis()
{
	dbgprintf("[GEN] Gensis running!\n");
	while(1)
	{
		print_memory_status();

		for (int i = 0; i < MAX_NUM_OF_PCBS; i++){
			if(pcbs[i].pid == -1 || pcbs[i].window == NULL)
				continue;
			
			draw_window(pcbs[i].window);
		}
		yield();
	}
}


void gensis2()
{
	while(1)
	{

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
	
	twriteln(" PID   Stack       Status       Name");
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
		twritef("  %d    0x%x    %s     %s bl: %d\n", pcbs[largest].pid, pcbs[largest].esp, status[pcbs[largest].running], pcbs[largest].name, pcbs[largest].blocked_count);
	}
	
}

void pcb_print_queues()
{
	twritef("Running queue (0x%x):\n ====> ", pcb_running_queue);
	struct pcb* next = pcb_running_queue;

	twritef("[%d]%s ->", next->pid, next->name);
	
	if(next->prev != pcb_running_queue)
		next = next->prev;

	while(next != pcb_running_queue){
		twritef("[%d]%s -> ", next->pid, next->name);
		next = next->prev;
	}
	
	twritef("\n\n");
	twritef("Blocked queue (0x%x):\n ====> ", pcb_blocked_queue);
	next = pcb_blocked_queue;
	while(next != NULL){
		twritef("[%d]%s ->", next->pid, next->name);
		next = next->next;
	}
	twritef("\n");
}


void pcb_set_blocked(int pid)
{
	if(pid < 0 || pid > MAX_NUM_OF_PCBS)
		return;

	pcbs[pid].running = BLOCKED;

	pcb_queue_remove(&pcbs[pid]);
	pcb_queue_push(&pcb_blocked_queue, &pcbs[pid], SINGLE_LINKED);

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
int pcb_cleanup(int pid)
{

	if(pid < 0 || pid > MAX_NUM_OF_PCBS)
		return -1;

	CLI();

	dbgprintf("[PCB] Cleaning zombie process %s\n", pcbs[pid].name);

	pcb_queue_remove(&pcbs[pid]);
	free((void*)pcbs[pid].org_stack);
	pcb_count--;
	
	memset(&pcbs[pid], 0, sizeof(struct pcb));

	pcbs[pid].running = STOPPED;
	pcbs[pid].pid = -1;
	pcbs[pid].window = NULL;
	
	STI();

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
int init_pcb(int pid, struct pcb* pcb, void (*entry)(), char* name)
{
	uint32_t stack = (uint32_t) alloc(stack_size);
	if((void*)stack == NULL)
	{
		dbgprintf("[PCB] STACK == NULL");
		return -1;
	}

	/* Stack grows down so we want the upper part of allocated memory.*/ 
	pcb->ebp = stack+stack_size-1;
	pcb->esp = stack+stack_size-1;
	pcb->k_esp = pcb->esp;
	pcb->k_ebp = pcb->k_esp;
	pcb->eip = entry;
	pcb->running = NEW;
	pcb->pid = pid;
	pcb->org_stack = stack;
	memcpy(pcb->name, name, strlen(name)+1);

	return 1;
}

int create_process(char* program)
{
	CLI();
	/* Load process from disk */
	inode_t inode = fs_open(program);

	char buf[MAX_FILE_SIZE];
	int read = fs_read((char* )buf, inode);
	fs_close(inode);
	dbgprintf("[INIT PROCESS] Reading %s from disk (%d bytes)\n", program, read);

	/* Create stack and pcb */
	 int i; /* Find a pcb is that is "free" */
	for(i = 0; i < MAX_NUM_OF_PCBS; i++)
		if(pcbs[i].running == STOPPED)
			break;
	
	struct pcb* pcb = &pcbs[i];

	char* pname = "program";

	pcb->eip = (void (*)()) 0x1000000; /* External programs start */
	pcb->running = NEW;
	pcb->pid = i;
	memcpy(pcb->name, pname, strlen(pname)+1);
	pcb->esp = 0xEFFFFFF0;
	pcb->ebp = pcb->esp;
	pcb->k_esp = (uint32_t) alloc(stack_size)+stack_size-1;
	pcb->k_ebp = pcb->k_esp;
	pcb->window = pcbs[2].window;
	//dbgprintf("[INIT PROCESS] Adding window %s\n", pcb->window->name);
	pcb->is_process = 1;

	dbgprintf("[INIT PROCESS] Setup PCB %d for %s\n", i, program);

	/* Memory map data */
	init_process_paging(pcb, buf, read);

	pcb_queue_push_running(pcb);

	pcb_count++;
	STI();
	dbgprintf("[INIT PROCESS] Created new process!\n");
	/* Run */
	return 1;
}

/**
 * @brief Add a pcb to the list of running proceses.
 * Also instantiates the PCB itself.
 * 
 * @param entry pointer to entry function.
 * @param name name of process
 * @return int amount of running processes, -1 on error.
 */
int add_pcb(void (*entry)(), char* name)
{   
	CLI();
	if(MAX_NUM_OF_PCBS == pcb_count)
		return -1;

	int i; /* Find a pcb is that is "free" */
	for(i = 0; i < MAX_NUM_OF_PCBS; i++)
		if(pcbs[i].running == STOPPED)
			break;
	
	int ret = init_pcb(i, &pcbs[i], entry, name);
	if(!ret) return ret;

	pcb_queue_push_running(&pcbs[i]);

	pcbs[i].page_dir = kernel_page_dir;
	pcbs[i].is_process = 0;

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
void init_pcbs()
{   

	/* Stopped processes are eligible to be "replaced." */
	for (int i = 0; i < MAX_NUM_OF_PCBS; i++)
	{
		pcbs[i].running = STOPPED;
		pcbs[i].pid = -1;
		pcbs[i].window = NULL;
	}
	current_running = &pcbs[0];
	pcbs[0].next = &pcbs[0];
	pcbs[0].prev = &pcbs[0];

	int ret = add_pcb(&gensis, "Gensis");
	if(ret < 0) return; // error

	ret = add_pcb(&gensis2, "Adam");
	if(ret < 0) return; // error
	dbgprintf("[PCB] All process control blocks are ready.\n");

}

void start_tasks()
{
	start_pcb();
	/* We will never reach this.*/
}
