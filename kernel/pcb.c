#include <pcb.h>
#include <screen.h>
#include <memory.h>

#define MAX_NUM_OF_PCBS 10
#define stack_size 0x2000

struct pcb pcbs[MAX_NUM_OF_PCBS];
struct pcb* current_running = NULL;
struct pcb* last_added = &pcbs[0];
int pcb_count = 0;

void pcb_function()
{
    int num = 0;
    int progress = 0;
	while(1){
		num = (num+1) % 100000000;
		if(num % 100000000 == 0)
		{  
            void* ptr = alloc(0x1000*(rand()%5+1));
            progress++;
			scrprintf(10, 11, "Process 1: %d", progress);
            free(ptr);
		}
	};
}

void pcb_function2()
{
    int num = 0;
    int progress = 0;
	while(1){
		num = (num+1) % 100000000;
		if(num % 100000000 == 0)
		{  
            void* ptr = alloc(0x1000*(rand()%5+1));
            progress++;
			scrprintf(30, 11, "Process 2: %d", progress);
            free(ptr);
		}
	};
}

uint32_t function_ptrs[] = {(uint32_t) &pcb_function, (uint32_t) &pcb_function2};

int init_pcb(int pid, struct pcb* pcb, uint32_t entry)
{
    uint32_t stack = (uint32_t) alloc(stack_size);
    if((void*)stack == NULL)
    {
        scrprintf(10, 14, "STACK == NULL");
        return -1;
    }

    /* Stack grows down so we want the upper part of allocated memory.*/ 
    pcb->ebp = stack+stack_size-1;
    pcb->esp = stack+stack_size-1;
    pcb->eip = entry;
    pcb->running = 0;
    pcb->pid = pid;

    return 1;
}

int add_pcb(uint32_t entry)
{   
    CLI();
    if(MAX_NUM_OF_PCBS == pcb_count)
    {
        return -1;
    }

    int ret = init_pcb(pcb_count, &pcbs[pcb_count], entry);
    if(!ret) return ret;

    /* Change the linked list so that the last (new) element points to start. */
    last_added->next = &pcbs[pcb_count];
    pcbs[pcb_count].next = &pcbs[0];
    last_added = &pcbs[pcb_count];

    pcb_count++;
    STI();
    return pcb_count;
}   

void start_pcb()
{   
    current_running->running = RUNNING;
    _start_pcb(); /* asm function */
}

void yield()
{
    _context_switch();
}

void context_switch()
{   
    current_running = current_running->next;
    if(!current_running->running)
    {
        start_pcb();
    }
}

void init_pcbs()
{   
    for (size_t i = 0; i < 2; i++)
    {   
        int ret = add_pcb(function_ptrs[i]);
        if(!ret) return; // error

        scrprintf(51, 18+i, "PCB %d: 0x%x, 0x%x\n", pcbs[i].pid, pcbs[i].esp, pcbs[i].eip);
    }
}

void start_tasks()
{
    current_running = &pcbs[0];
    start_pcb();

    /* We will never reach this.*/
}