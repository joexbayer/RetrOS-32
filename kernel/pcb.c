#include <pcb.h>
#include <screen.h>

#define NUM_OF_PCBS 2
#define stack_size 0x2000
#define mem_start 0x100000

int stack_used = 0; // temp while no memory management

pcb_t pcbs[NUM_OF_PCBS];
pcb_t *current_running = NULL;

void pcb_function()
{
    int num = 0;
    int progress = 0;
	while(1){
		num = (num+1) % 100000000;
		if(num % 100000000 == 0)
		{  
            progress++;
			scrprintf(10, 11, "Process 1: %d", progress);
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
            progress++;
			scrprintf(30, 11, "Process 2: %d", progress);
		}
	};
}

uint32_t function_ptrs[] = {(uint32_t) &pcb_function, (uint32_t) &pcb_function2};

int init_pcb(int pid, pcb_t* pcb, uint32_t entry)
{
    uint32_t stack = mem_start+stack_size+stack_used;
    stack_used += stack_size;
    pcb->ebp = stack;
    pcb->esp = stack;
    pcb->eip = entry;
    pcb->running = 0;
    pcb->pid = pid;

    return 1;
}

void start_multitasks()
{
    current_running->running = RUNNING;
    _start_pcb();
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
        start_multitasks();
    }
}

void init_pcbs()
{   
    current_running = &pcbs[0];
    for (size_t i = 0; i < NUM_OF_PCBS; i++)
    {   
        init_pcb(i, &pcbs[i], function_ptrs[i]);
        scrprintf(51, 18+i, "PCB %d: 0x%x, 0x%x\n", pcbs[i].pid, pcbs[i].esp, pcbs[i].eip);
        
        if(i == NUM_OF_PCBS-1)
        {
            pcbs[i].next = &pcbs[0];
            continue;
        }
        pcbs[i].next = &pcbs[i+1];
    }
    start_multitasks();
}