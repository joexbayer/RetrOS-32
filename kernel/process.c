/**
 * @file process.c
 * @author Joe Bayer (joexbayer)
 * @brief Simple abstraction to easily add programs and their functions to the terminal.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <process.h>
#include <terminal.h>
#include <pcb.h>

#define MAX_PROCESSES 10

static struct process processes[MAX_PROCESSES];
static int process_count = 0;
struct process* current_process = &processes[0];


/**
 * @brief Attaches a function to the given process ID. Making it visable to the shell.
 * 
 * @param id of the process, given by ATTACH_PROCESS
 * @param name name of the function.
 * @param fn pointer to the function
 * @return int 1 on success, -1 on error.
 */
int ATTACH_FUNCTION(int id, char* name, uint32_t* fn)
{
    struct process* proc = &processes[id];
    if(proc->total_functions == MAX_PROCESS_FUNCTIONS)
        return -1;

    if(strlen(name)+1 > MAX_FUNCTION_NAME)
        return -1;

    memcpy(proc->functions[proc->total_functions].name, name, strlen(name)+1);
    proc->functions[proc->total_functions].f = fn;
    proc->total_functions++;
    return 1;
}

/**
 * @brief Attaches process to the global process list. Making it accessible by the shell.
 * 
 * @param name name of the process.
 * @param entry pointer to the processes main function.
 * @return int process id, -1 on error.
 */
int ATTACH_PROCESS(char* name, uint32_t* entry)
{
    if(process_count == MAX_PROCESSES)
        return -1;

    if(strlen(name)+1 > MAX_PROCESS_NAME)
        return -1;

    struct process* proc = &processes[process_count];
    memcpy(proc->name, name, strlen(name)+1);
    proc->entry = entry;
    proc->pid = -1;
    proc->focus = NOFOCUS;
    proc->total_functions = 0;

    for (size_t i = 0; i < MAX_PROCESS_INSTANCES; i++)
    {
        proc->instances[i] = -1;
    }

    return process_count++;
}


void start_process(int id)
{

    if(id > process_count-1){
        twriteln("Process does not exist.");    
        return;
    }

    int i;
    for (i = 0; i < MAX_PROCESS_INSTANCES ; i++)
    {
        if(processes[id].instances[i] == -1)
            break;
    }
    
    processes[id].instances[i] = add_pcb(processes[id].entry, processes[id].name);

    twriteln("Process Started.");
}

void stop_process(char* name)
{
    
}

void list_functions()
{
    twritef("Functions for %s:\n", current_process->name);
    for (size_t i = 0; i < current_process->total_functions; i++)
    {
        twritef("   %s\n", current_process->functions[i].name);
    }
    
}

void list_instances()
{
    twritef("Instances of %s:\n", current_process->name);
    int i;
    for (i = 0; i < MAX_PROCESS_INSTANCES ; i++)
    {
        if(current_process->instances[i] != -1)
            twritef("PID: %d, instance: %d\n", current_process->instances[i], i);
    }
}

void switch_process(int id)
{
    if(id > process_count-1){
        twriteln("Process does not exist.");    
        return;
    }
    twriteln("Switched Process..");   
    current_process = &processes[id];
}

void list_processes()
{
    for (size_t i = 0; i < process_count; i++)
    {
        twritef("%d: %s\n", i, processes[i].name);
    }
}