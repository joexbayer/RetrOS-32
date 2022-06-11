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
#include <screen.h>
#include <memory.h>
#include <sync.h>
#include <timer.h>
#include <net/netdev.h>

#define MAX_NUM_OF_PCBS 10
#define stack_size 0x2000

 char* status[] = {"STOPPED", "RUNNING"};

static struct pcb pcbs[MAX_NUM_OF_PCBS];
struct pcb* current_running = NULL;
static int pcb_count = 0;

void pcb_function()
{
	while(1)
    {
        print_pcb_status();
        print_memory_status();
        netdev_print_status();
        networking_print_status();
		sleep(1);
	}
}

void gensis()
{
    while(1);
}

/**
 * @brief Main function of the PCB background process.
 * Printing out information about the currently running processes.
 */
void print_pcb_status()
{
    int width = (SCREEN_WIDTH/3)+(SCREEN_WIDTH/6)-1 + (SCREEN_WIDTH/6);
    int height = (SCREEN_HEIGHT/2 + SCREEN_HEIGHT/5)+1;

    for (size_t i = 0; i < MAX_NUM_OF_PCBS; i++)
    {
        if(pcbs[i].pid == -1)
            continue;

        switch (pcbs[i].running)
        {
        case RUNNING:
            scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_GREEN);
            break;
        case BLOCKED:
            scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_BROWN);
            break;
        case STOPPED:
            scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_RED);
            break;
        case SLEEPING:
            scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY);
            break;
        default:
            continue;
        }

        scrprintf(width, height+i, "PID %d: %s. 0x%x",pcbs[i].pid, pcbs[i].name, pcbs[i].esp);
        /* code */
        scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
}

/**
 * @brief Sets the process with given pid to stopped. Also frees the process's stack.
 * 
 * @param pid id of the process.
 * @return int index of pcb, -1 on error.
 */
int stop_task(int pid)
{
    /* 
        FIXME: BUG, after a task is stopped, the next started task will fill its place.
        BUT, the next task after that will have the same stack as a other PCB.
     */
    for (int i = 0; i < pcb_count; i++)
    {
        if(pcbs[i].pid == pid)
        {
            CLI();
            pcbs[i].running = STOPPED;
            free((void*)pcbs[i].org_stack);
            pcb_count--;
            pcbs[i].prev->next = pcbs[i].next;
            pcbs[i].next->prev = pcbs[i].prev;
            STI();
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
int init_pcb(int pid, struct pcb* pcb, void (*entry)(), char* name)
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
    pcb->org_stack = stack;
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

    struct pcb* next = current_running->next;
    current_running->next = &pcbs[i];
    next->prev = &pcbs[i];
    pcbs[i].next = next;
    pcbs[i].prev = current_running;

    pcb_count++;
    STI();
    return i;
}   

void start_pcb()
{   
    current_running->running = RUNNING;
    _start_pcb(); /* asm function */
}


void sleep(int time)
{
    current_running->sleep_time = get_time() + time;
    current_running->running = SLEEPING;
    yield();
}

void yield()
{
    CLI();
    _context_switch();
}

void exit()
{
    CLI();
    current_running->running = STOPPED;
    free((void*)current_running->org_stack);
    pcb_count--;
    current_running->prev->next = current_running->next;
    current_running->next->prev = current_running->prev;

    _context_switch();
}

void block()
{
    current_running->running = BLOCKED;
    _context_switch();
}

void unblock(int pid)
{
    int i;
    for(i = 0; i < MAX_NUM_OF_PCBS; i++)
        if(pcbs[i].pid == pid && pcbs[i].running == BLOCKED)
            pcbs[i].running = RUNNING;
}

void context_switch()
{   
    current_running = current_running->next;
    while(current_running->running != RUNNING)
    {
        switch (current_running->running)
        {
        case STOPPED:
            current_running = current_running->next;
            break;
        case NEW:
            start_pcb();
            break; /* Never reached. */
        case SLEEPING:
            if(get_time() >= current_running->sleep_time)
                current_running->running = RUNNING;
            else
                current_running = current_running->next;
            break;
        default:
            current_running = current_running->next;
            break;
        }
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
        pcbs[i].pid = -1;
    }

    pcbs[0].next = &pcbs[0];
    pcbs[0].prev = &pcbs[0];

    int ret = add_pcb(&gensis, "Gensis");
    if(ret < 0) return; // error

    ret = add_pcb(&pcb_function, "PCBd");
    if(ret < 0) return; // error
}

void start_tasks()
{
    current_running = &pcbs[0];
    start_pcb();

    /* We will never reach this.*/
}