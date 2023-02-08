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
#include <gfx/gfxlib.h>

#define STACK_SIZE 0x2000

static const char* status[] = {"stopped ", "running ", "new     ", "blocked ", "sleeping", "zombie"};

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
		;
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

void Genesis()
{
	dbgprintf("[GEN] Genesis running!\n");
	struct gfx_window* window = gfx_new_window(400, 100);
	while(1)
	{
		gfx_draw_rectangle(0, 0, 400, 100, VESA8_COLOR_LIGHT_GRAY5);
		gfx_draw_rectangle(3, 3, 395, 66, VESA8_COLOR_LIGHT_GRAY1);

		gfx_line(2, 2, 66, GFX_LINE_OUTER_VERTICAL, VESA8_COLOR_BLUE);
		gfx_line(3, 2, 396, GFX_LINE_INNER_HORIZONTAL, VESA8_COLOR_BLUE);

		gfx_line(396, 2, 66, GFX_LINE_INNER_VERTICAL, VESA8_COLOR_BLUE);
		gfx_line(2, 68, 396, GFX_LINE_OUTER_HORIZONTAL, VESA8_COLOR_BLUE);

		print_pcb_status();

		gfx_draw_format_text(10, 80, VESA8_COLOR_BLACK, "Total: %d", pcb_count);
		
		gfx_commit();
		/*print_memory_status();

		for (int i = 0; i < MAX_NUM_OF_PCBS; i++){
			if(pcbs[i].pid == -1 || pcbs[i].window == NULL)
				continue;
			
			draw_window(pcbs[i].window);
		}*/
		sleep(200);
	}
}


void system_info()
{
	static const char* SIZES[] = { "B", "kB", "MB", "GB" };
	int memory_info = 106;
	struct gfx_window* window = gfx_new_window(225, 375);
	while(1)
	{
		gfx_draw_rectangle(0, 0, 225, 375, VESA8_COLOR_LIGHT_GRAY5);
		
		gfx_draw_rectangle(2, 98, 90, 106, VESA8_COLOR_BLACK);
		gfx_draw_rectangle(5, 100, 25, 100, VESA8_COLOR_DARK_GREEN);

		gfx_draw_text(60, 90, "Memory", VESA8_COLOR_BLACK);
		int mem_dyn = memory_dynamic_usage();
		gfx_draw_rectangle(5, 200-mem_dyn/10, 25, mem_dyn/10, VESA8_COLOR_GREEN);

		gfx_draw_rectangle(34, 100, 25, 100, VESA8_COLOR_DARK_GREEN);
		int perm_dyn = memory_permanent_usage();
		gfx_draw_rectangle(34, 200-perm_dyn/10, 25, perm_dyn/10, VESA8_COLOR_GREEN);

		gfx_draw_rectangle(63, 100, 25, 100, VESA8_COLOR_DARK_GREEN);
		gfx_draw_rectangle(63, 200-(memory_pages_usage()/2.5), 25, memory_pages_usage()/2.5, VESA8_COLOR_GREEN);

		gfx_inner_box(2, 98, 90, 106, 0);	

		/*while (used >= 1024 && div_used < (sizeof SIZES / sizeof *SIZES)) {
            div_used++;   
            used /= 1024;
        }*/

		gfx_draw_rectangle(96, memory_info-4, 125, 30, VESA8_COLOR_LIGHT_GRAY1);
		gfx_inner_box(96, memory_info-4, 125, 30, 0);

		gfx_draw_format_text(100, memory_info, VESA8_COLOR_BLACK, "Dynamic:");
		gfx_draw_format_text(100, memory_info+8, VESA8_COLOR_BLACK, "Used      %dkb", mem_dyn);
		gfx_draw_format_text(100, memory_info+16, VESA8_COLOR_BLACK, "Free      %dkb", memory_dynamic_total()-mem_dyn);
		
		gfx_draw_rectangle(96, memory_info+30, 125, 28, VESA8_COLOR_LIGHT_GRAY1);
		gfx_inner_box(96, memory_info+30, 125, 28, 0);	

		gfx_draw_format_text(100, memory_info+32, VESA8_COLOR_BLACK, "Permanent:");
		gfx_draw_format_text(100, memory_info+40, VESA8_COLOR_BLACK, "Used      %dkb", perm_dyn);
		gfx_draw_format_text(100, memory_info+48, VESA8_COLOR_BLACK, "Free      %dkb", memory_permanent_total()-perm_dyn);

		gfx_draw_rectangle(96, memory_info+62, 125, 30, VESA8_COLOR_LIGHT_GRAY1);
		gfx_inner_box(96, memory_info+62, 125, 30, 0);	

		gfx_draw_format_text(100, memory_info+64, VESA8_COLOR_BLACK, "Pages:");
		gfx_draw_format_text(100, memory_info+64+8, VESA8_COLOR_BLACK, "Used        %s%d", memory_pages_usage() > 99 ? " " : "" ,  memory_pages_usage());
		gfx_draw_format_text(100, memory_info+64+18, VESA8_COLOR_BLACK, "Free        %d", memory_pages_total());


		gfx_draw_rectangle(96, memory_info+98, 125, 30, VESA8_COLOR_LIGHT_GRAY1);
		gfx_inner_box(96, memory_info+98, 125, 30, 0);	
		gfx_draw_format_text(100, memory_info+100, VESA8_COLOR_BLACK, "Process:");
		gfx_draw_format_text(100, memory_info+100+8, VESA8_COLOR_BLACK, "Used      %dkb", memory_process_usage()/1024);
		gfx_draw_format_text(100, memory_info+100+18, VESA8_COLOR_BLACK, "Free       %dmb", memory_process_total()/1024/1024);

		//gfx_outer_box(100, 200, 70, 20, 1);
		gfx_draw_format_text(5, 210, VESA8_COLOR_BLACK, "Total:");
		gfx_draw_format_text(5, 220, VESA8_COLOR_BLACK, "%dkb/%dmb", (mem_dyn*MEM_CHUNK+perm_dyn*MEM_CHUNK+(memory_pages_usage()*4096)+memory_process_usage())/1024, (0x100000*15)/1024/1024);


		for (int i= 0; i < 20; i++)
			gfx_line(5, 100+(i*5), 85, GFX_LINE_HORIZONTAL, VESA8_COLOR_BLACK);

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
	
	gfx_draw_text(10, 7, "Name           Status    Memory    Stack     PID", VESA8_COLOR_LIGHT_BROWN);
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
		//gfx_draw_format_text(10, 10+done_list_count*8, VESA8_COLOR_BLACK, " %d  0x%x  %s  %s  %s\n", pcbs[largest].pid, pcbs[largest].used_memory, status[pcbs[largest].running], pcbs[largest].is_process == 1 ? "Process" : "kthread", pcbs[largest].name);

		gfx_draw_format_text(10, 10+done_list_count*8, VESA8_COLOR_BLACK, "%s", pcbs[largest].name);
		gfx_draw_format_text(10 + 15*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%s",status[pcbs[largest].running]);
		gfx_draw_format_text(10+15*8+10*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%d", pcbs[largest].used_memory);
		gfx_draw_format_text(10+15*8+10*8 + 10*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "0x%x", pcbs[largest].esp);
		gfx_draw_format_text(10+15*8+10*8+10*8+11*8, 10+done_list_count*8, VESA8_COLOR_BLACK, "%d", pcbs[largest].pid);
	}
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

	if(pcbs[pid].gfx_window != NULL)
		gfx_destory_window(pcbs[pid].gfx_window);

	pcb_queue_remove(&pcbs[pid]);
	dbgprintf("[PCB] Cleanup on PID %d stack: 0x%x (original: 0x%x)\n", pid, pcbs[pid].esp, pcbs[pid].org_stack);
	dbgprintf("[PCB] Cleanup on PID %d kstack: 0x%x\n", pid, pcbs[pid].k_esp);

	struct allocation* iter = pcbs[pid].allocations;
	while(iter != NULL)
	{
		struct allocation* next = iter->next;
		kfree(iter);
		iter = next;
	}
	
	pcb_count--;
	
	if(pcbs[pid].is_process)
		cleanup_process_paging(&pcbs[pid]);
	else
		kfree((void*)pcbs[pid].org_stack);

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
	uint32_t stack = (uint32_t) kalloc(STACK_SIZE);
	if((void*)stack == NULL)
	{
		dbgprintf("[PCB] STACK == NULL");
		return -1;
	}

	/* Stack grows down so we want the upper part of allocated memory.*/ 
	pcb->ebp = stack+STACK_SIZE-1;
	pcb->esp = stack+STACK_SIZE-1;
	pcb->k_esp = pcb->esp;
	pcb->k_ebp = pcb->k_esp;
	pcb->eip = entry;
	pcb->running = NEW;
	pcb->pid = pid;
	pcb->org_stack = stack;
	pcb->allocations = NULL;
	pcb->used_memory = 0;

	memcpy(pcb->name, name, strlen(name)+1);

	return 1;
}

int create_process(char* program)
{
	CLI();
	/* Load process from disk */
	inode_t inode = fs_open(program);
	if(inode == 0)
		return 0;

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
	pcb->k_esp = (uint32_t) kalloc(STACK_SIZE)+STACK_SIZE-1;
	dbgprintf("[INIT PROCESS] Setup PCB %d for %s\n", i, program);
	pcb->k_ebp = pcb->k_esp;
	//pcb->window = pcbs[2].window;
	pcb->term = current_running->term;
	//dbgprintf("[INIT PROCESS] Adding window %s\n", pcb->window->name);
	pcb->is_process = 1;

	/* Memory map data */
	init_process_paging(pcb, buf, read);

	pcb_queue_push_running(pcb);

	pcb_count++;
	STI();
	dbgprintf("[INIT PROCESS] Created new process!\n");
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

	int ret = add_pcb(&Genesis, "Genesis");
	if(ret < 0) return; // error

	dbgprintf("[PCB] All process control blocks are ready.\n");

}

void start_tasks()
{
	start_pcb();
	/* We will never reach this.*/
}
