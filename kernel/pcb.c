#include <pcb.h>
#include <screen.h>
#include <memory.h>
#include <keyboard.h>

#define MAX_NUM_OF_PCBS 10
#define stack_size 0x2000

 char* status[] = {"STOPPED", "RUNNING"};

static struct pcb pcbs[MAX_NUM_OF_PCBS];
struct pcb* current_running = NULL;
static struct pcb* last_added = &pcbs[0];
static int pcb_count = 0;

void pcb_function()
{
    int num = 0;
    int progress = 0;

	while(1)
    {
		num = (num+1) % 100000000;
        //char c = kb_get_char();
		if(num % 100000000 == 0)
		{  
            void* ptr = alloc(0x1000*(rand()%5+1));
            progress++;
			scrprintf(10, 12, "Process 1: %d", progress);
            print_pcb_status();
            free(ptr);
		}
	};
}

/**
 * @brief Main function of the PCB background process.
 * Printing out information about the currently running processes.
 */
void print_pcb_status()
{
    int width = (SCREEN_WIDTH/3)+(SCREEN_WIDTH/6)-1 + (SCREEN_WIDTH/6);
    int height = (SCREEN_HEIGHT/2 + SCREEN_HEIGHT/5)+1;

    for (size_t i = 0; i < pcb_count; i++)
    {
        scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_GREEN);
        if(pcbs[i].running != RUNNING)
            scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_RED);

        scrprintf(width, height+i, "PID %d: %s, SP: 0x%x",pcbs[i].pid, pcbs[i].name, pcbs[i].esp);
        /* code */
        scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
}

void pcb_function2()
{
    int num = 0;
    int progress = 0;
	while(1)
    {
		num = (num+1) % 100000000;
		if(num % 100000000 == 0)
		{  
            void* ptr = alloc(0x1000*(rand()%5+1));
            progress++;
			scrprintf(10, 13, "Process 2: %d", progress);
            free(ptr);
		}
	};
}

static uint32_t function_ptrs[] = {(uint32_t) &pcb_function, (uint32_t) &pcb_function2};
/**
 * @brief Sets the process with given pid to stopped. Also frees the process's stack.
 * 
 * @param pid id of the process.
 * @return int index of pcb, -1 on error.
 */
int stop_task(int pid)
{
    for (size_t i = 0; i < pcb_count; i++)
    {
        if(pcbs[i].pid == pid)
        {
            pcbs[i].running = STOPPED;
            free(pcbs[i].ebp-stack_size+1);
            pcb_count--;
            return i;
        }
    }
    return -1;
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
int init_pcb(int pid, struct pcb* pcb, uint32_t entry, char* name)
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
    pcb->running = NEW;
    pcb->pid = pid;
    memcpy(pcb->name, name, strlen(name)+1);

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
int add_pcb(uint32_t entry, char* name)
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


    /* Change the linked list so that the last (new) element points to start. */
    last_added->next = &pcbs[i];
    pcbs[i].next = &pcbs[0];
    last_added = &pcbs[i];

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
    while(current_running->running == STOPPED)
        current_running = current_running->next;

    if(current_running->running == NEW)
    {
        start_pcb();
    }
}

/**
 * @brief Sets all PCBs state to stopped. Meaning they can be started.
 * Also starts the PCB background process.
 */
void init_pcbs()
{   
    /* Stopped processes are eligible to be "replaced." */
    for (size_t i = 0; i < MAX_NUM_OF_PCBS; i++)
    {
        pcbs[i].running = STOPPED;
    }

    for (size_t i = 0; i < 2; i++)
    {   
        int ret = add_pcb(function_ptrs[i], "PCBd");
        if(!ret) return; // error

        //scrprintf(51, 18+i, "PCB %d: 0x%x, 0x%x\n", pcbs[i].pid, pcbs[i].esp, pcbs[i].eip);
    }
}

void start_tasks()
{
    current_running = &pcbs[0];
    start_pcb();

    /* We will never reach this.*/
}