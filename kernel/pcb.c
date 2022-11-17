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

 char* status[] = {"stopped ", "running ", "new     ", "blocked ", "sleeping"};

static struct pcb pcbs[MAX_NUM_OF_PCBS];
struct pcb* current_running = NULL;
static int pcb_count = 0;

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
        for (int i = 0; i < 10000000; i++)
        { 
            int random = rand();
            void* ptr = alloc(random * 5120 % 10000);
            free(ptr);
            yield();
        }

        yield();
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
        twritef("  %d    0x%x    %s     %s\n", pcbs[largest].pid, pcbs[largest].esp, status[pcbs[largest].running], pcbs[largest].name);
    }
    
}

void pcb_set_running(int pid)
{
    int i;
    for(i = 0; i < MAX_NUM_OF_PCBS; i++)
        if(pcbs[i].pid == pid){
            pcbs[i].running = RUNNING;
            return;
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

            pcbs[i].esp = 0;
            pcbs[i].ebp = 0;
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

    pcb->eip = (void (*)()) 0x1000000; /* External programs start */
    pcb->running = NEW;
    pcb->pid = i;
    memcpy(pcb->name, program, strlen(program)+1);
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

    struct pcb* next = current_running->next;
    current_running->next = &pcbs[i];
    next->prev = &pcbs[i];
    pcbs[i].next = next;
    pcbs[i].prev = current_running;

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

    struct pcb* next = current_running->next;
    current_running->next = &pcbs[i];
    next->prev = &pcbs[i];
    pcbs[i].next = next;
    pcbs[i].prev = current_running;
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

    //int ret = add_pcb(&gensis2, "Adam");
    //if(ret < 0) return; // error
    dbgprintf("[PCB] All process control blocks are ready.\n");

}

void start_tasks()
{
    start_pcb();
    /* We will never reach this.*/
}
