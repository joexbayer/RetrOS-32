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

//#include <gfx/gfxlib.h>

#define STACK_SIZE 0x2000
static const char* pcb_status[] = {"stopped ", "running ", "new     ", "blocked ", "sleeping", "zombie"};

/* Prototype functions for pcb queue interface */
static void pcb_queue_push(struct pcb_queue* queue, struct pcb* pcb);
static void pcb_queue_add(struct pcb_queue* queue, struct pcb* pcb);
static void pcb_queue_remove(struct pcb_queue* queue, struct pcb* pcb);
static struct pcb* pcb_queue_pop(struct pcb_queue* queue);

/* Setup for default pcb queue operations */
static struct pcb_queue_operations pcb_queue_default_ops = {
	.push = &pcb_queue_push,
	.add = &pcb_queue_add,
	.remove = &pcb_queue_remove,
	.pop = &pcb_queue_pop
};

/* Global running and blocked queue */
struct pcb_queue* running;
struct pcb_queue* blocked;


/* Helper function to attach default ops */
void pcb_queue_attach_ops(struct pcb_queue* q)
{
	q->ops = &pcb_queue_default_ops;
}

/**
 * @brief Creates a new PCB queue.
 *
 * The `pcb_new_queue()` function allocates memory for a new `pcb_queue` structure and initializes its members.
 * The `_list` member is set to `NULL`, and the queue's operations and spinlock are attached and initialized.
 *
 * @return A pointer to the newly created `pcb_queue` structure.
 */ 
struct pcb_queue* pcb_new_queue()
{
	struct pcb_queue* queue = kalloc(sizeof(struct pcb_queue));
	queue->_list = NULL;
	pcb_queue_attach_ops(queue);
	queue->spinlock = 0;
	queue->total = 0;

	return queue;
}

/**
 * @brief Pushes a PCB onto the PCB queue.
 *
 * The `pcb_queue_push()` function adds a PCB to the end of the specified queue. The function takes a pointer to
 * the `pcb_queue` structure and a pointer to the `pcb` structure to be added as arguments. The function uses
 * a spinlock to protect the critical section and adds the PCB to the end of the queue by traversing the current
 * list of PCBs and adding the new PCB to the end.
 *
 * @param queue A pointer to the `pcb_queue` structure to add the `pcb` to.
 * @param pcb A pointer to the `pcb` structure to add to the queue.
 */
static void pcb_queue_push(struct pcb_queue* queue, struct pcb* pcb)
{
	assert(queue != NULL);

	SPINLOCK(queue, {

		struct pcb* current = queue->_list;
		if(current == NULL)
		{
			queue->_list = pcb;
			break;
		}
		while (current->next == NULL)
			current = current->next;
		current->next = pcb;
		pcb->next = NULL;
		queue->total++;

	});
	dbgprintf("Added %s to a queue\n", pcb->name);
}

/**
 * @brief Adds a PCB to the PCB queue.
 *
 * The `pcb_queue_add()` function adds a PCB to the beginning of the specified queue. The function takes a pointer to
 * the `pcb_queue` structure and a pointer to the `pcb` structure to be added as arguments. The function uses
 * a spinlock to protect the critical section and adds the PCB to the beginning of the queue by modifying the pointers
 * of the existing PCBs in the queue to insert the new PCB at the front.
 *
 * @param queue A pointer to the `pcb_queue` structure to add the `pcb` to.
 * @param pcb A pointer to the `pcb` structure to add to the queue.
 */
static void pcb_queue_add(struct pcb_queue* queue, struct pcb* pcb)
{
	assert(queue != NULL);

	SPINLOCK(queue, {

		struct pcb* prev = queue->_list->prev;
		queue->_list->prev = pcb;
		pcb->next = queue->_list;
		prev->next = pcb;
		pcb->prev = prev;

		queue->total++;

	});
	dbgprintf("New pcb added to a queue\n");
}

/**
 * @brief Removes a PCB from the PCB queue.
 *
 * The `pcb_queue_remove()` function removes a PCB from the specified queue. The function takes a pointer to the
 * `pcb_queue` structure and a pointer to the `pcb` structure to be removed as arguments. The function uses a
 * spinlock to protect the critical section and removes the specified PCB from the queue by modifying the pointers
 * of the previous and next PCBs in the queue to bypass the removed PCB.
 *
 * @param queue A pointer to the `pcb_queue` structure to remove the `pcb` from.
 * @param pcb A pointer to the `pcb` structure to remove from the queue.
 */
static void pcb_queue_remove(struct pcb_queue* queue, struct pcb* pcb)
{
	assert(queue != NULL);

	SPINLOCK(queue, {

		struct pcb* prev = pcb->prev;
		prev->next = pcb->next;
		pcb->next->prev = prev;

		pcb->next = NULL;
		pcb->prev = NULL;

		queue->total--;
		
	});
}

/**
 * @brief Removes and returns the first PCB in the PCB queue.
 *
 * The `pcb_queue_pop()` function removes and returns the first PCB in the specified queue. The function takes a pointer
 * to the `pcb_queue` structure as an argument. The function uses a spinlock to protect the critical section and removes
 * the first PCB from the queue by modifying the pointers of the previous and next PCBs in the queue to bypass the
 * removed PCB. The function returns a pointer to the removed PCB, or `NULL` if the queue is empty.
 *
 * @param queue A pointer to the `pcb_queue` structure to remove the first PCB from.
 * @return A pointer to the first PCB in the queue, or `NULL` if the queue is empty.
 */
static struct pcb* pcb_queue_pop(struct pcb_queue* queue)
{
    assert(queue != NULL);

	if(queue->_list == NULL)
		return NULL;

	struct pcb* front = NULL;
	SPINLOCK(queue, {

		front = queue->_list;
		queue->_list = front->next;
		if (queue->_list != NULL) {
			queue->_list->prev = NULL;
		}
		front->next = NULL;
		front->prev = NULL;
	});

    return front;
}

static struct pcb pcb_table[MAX_NUM_OF_PCBS];
static int pcb_count = 0;

/**
 * Current running PCB, used for context aware
 * functions such as windows drawing to the screen.
 */
struct pcb* current_running = &pcb_table[0];

/**
 * @brief Wrapper function to push to running queue
 * 
 * @param pcb 
 */
void pcb_queue_push_running(struct pcb* pcb)
{
	running->ops->add(running, pcb);
}

void pcb_queue_remove_running(struct pcb* pcb)
{
	running->ops->remove(running, pcb);
}

struct pcb* pcb_get_new_running()
{
	assert(running->_list != NULL);
	return running->_list;
}

/**
 * @brief Main function of the PCB background process.
 * Printing out information about the currently running processes.
void print_pcb_status()
{
	int done_list[MAX_NUM_OF_PCBS];
	int done_list_count = 0;
	
	__gfx_draw_text(10, 7, "Name           Status    Memory    Stack     PID", VESA8_COLOR_LIGHT_BROWN);
	gfx_line(10, 7+9, 382, GFX_LINE_HORIZONTAL, VESA8_COLOR_GRAY2);
	for (int i = 0; i < MAX_NUM_OF_PCBS; i++)
	{
		if(pcb_table[i].pid == -1)
			continue;

		int largest = 0;
		uint32_t largest_amount = 0;
		for (int j = 0; j < MAX_NUM_OF_PCBS; j++)
		{
			if(pcb_table[j].pid == -1)
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
			

			if(pcb_table[j].ebp-pcb_table[j].esp >= largest_amount){
				largest_amount = pcb_table[j].ebp-pcb_table[j].esp;
				largest = j;
			}
		}

		done_list[done_list_count] = largest;
		done_list_count++;
		//__gfx_draw_format_text(10, 10+done_list_count*8, VESA8_COLOR_BLACK, " %d  0x%x  %s  %s  %s\n", pcb_table[largest].pid, pcb_table[largest].used_memory, status[pcb_table[largest].running], pcb_table[largest].is_process == 1 ? "Process" : "kthread", pcb_table[largest].name);

		__gfx_draw_format_text(10, 10+done_list_count*8, VESA8_COLOR_BLACK, "%s", pcb_table[largest].name);
		__gfx_draw_format_text(10 + 15*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%s", pcb_status[pcb_table[largest].running]);
		__gfx_draw_format_text(10+15*8+10*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%d", pcb_table[largest].used_memory);
		__gfx_draw_format_text(10+15*8+10*8 + 10*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "0x%x", pcb_table[largest].esp);
		__gfx_draw_format_text(10+15*8+10*8+10*8+11*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%d", pcb_table[largest].pid);
	}
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
} */


void Genesis()
{
	while (1);
}

void idletask(){
	while(1){
		HLT();
		//yield();
	};
}


void dummytask(){
	while(1){
		char j = 0;
		for (int i = 0; i < 99999999; i++)
		{
			j = (j+1) % 10;
		}
	};
}

void pcb_set_running(int pid)
{
	if(pid < 0 || pid > MAX_NUM_OF_PCBS)
		return;

	pcb_table[pid].running = RUNNING;
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
	assert(pid != current_running->pid && !(pid < 0 || pid > MAX_NUM_OF_PCBS));

	gfx_destory_window(pcb_table[pid].gfx_window);

	running->ops->remove(running, &pcb_table[pid]);

	/* Free potential arguments */
	if(pcb_table[pid].argv != NULL){
		for (int i = 0; i < 5 /* Change to MAX_ARGS */; i++)
			kfree(pcb_table[pid].argv[i]);
		kfree(pcb_table[pid].argv);
	}	

	dbgprintf("[PCB] Cleanup on PID %d stack: 0x%x (original: 0x%x)\n", pid, pcb_table[pid].esp, pcb_table[pid].stack_ptr);
	
	if(pcb_table[pid].is_process){
		vmem_cleanup_process(&pcb_table[pid]);
	}
	kfree((void*)pcb_table[pid].stack_ptr);

	pcb_count--;

	memset(&pcb_table[pid], 0, sizeof(struct pcb));
	pcb_table[pid].running = STOPPED;
	pcb_table[pid].pid = -1;

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
	pcb->args = 0;
	pcb->argv = NULL;

	memcpy(pcb->name, name, strlen(name)+1);

	dbgprintf("[INIT KTHREAD] Created new kernel thread!\n");

	return 1;
}

int pcb_create_process(char* program, int args, char** argv)
{
	CLI();
	/* Load process from disk */
	inode_t inode = fs_open(program);
	if(inode == 0)
		return 0;

	dbgprintf("[INIT PROCESS] Reading %s from disk\n", program);
	char* buf = kalloc(MAX_FILE_SIZE);
	int read = fs_read(inode, buf, MAX_FILE_SIZE);
	fs_close(inode);

	/* Create stack and pcb */
	 int i; /* Find a pcb is that is "free" */
	for(i = 0; i < MAX_NUM_OF_PCBS; i++)
		if(pcb_table[i].running == STOPPED)
			break;
		
	assert(pcb_table[i].running == STOPPED);
	
	struct pcb* pcb = &pcb_table[i];

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
	pcb->args = args;
	pcb->argv = argv;

	/* Memory map data */
	vmem_init_process(pcb, buf, read);

	running->ops->add(running, pcb);

	pcb_count++;
	STI();
	kfree(buf);
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
		if(pcb_table[i].running == STOPPED)
			break;
	
	int ret = pcb_init_kthread(i, &pcb_table[i], entry, name);

	running->ops->add(running, &pcb_table[i]);

	pcb_count++;
	dbgprintf("Added %s, PID: %d, Stack: 0x%x\n", name, i, pcb_table[i].kesp);
	STI();
	return i;
}

void start_pcb()
{   
	current_running->running = RUNNING;
	dbgprintf("[START PCB] Starting pcb!\n");
	_start_pcb(); /* asm function */
	
	UNREACHABLE();
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
		pcb_table[i].running = STOPPED;
		pcb_table[i].pid = -1;
	}

	running = pcb_new_queue();
	blocked = pcb_new_queue();

	//int ret = pcb_create_kthread(&Genesis, "Genesis");
	//if(ret < 0) return; // error

	current_running = &pcb_table[0];
	pcb_table[0].next = &pcb_table[0];
	pcb_table[0].prev = &pcb_table[0];

	running->_list = current_running;

	dbgprintf("[PCB] All process control blocks are ready.\n");

}

void pcb_start()
{
	start_pcb();
	/* We will never reach this.*/
}
