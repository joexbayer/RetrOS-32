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
#include <screen.h>
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
int ATTACH_FUNCTION(int id, char* name, void (*fn)())
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
int ATTACH_PROCESS(char* name, void (*entry)())
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

    for (int i = 0; i < MAX_PROCESS_INSTANCES; i++)
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
}

void stop_process(char* name)
{
    
}

void list_functions()
{
    twritef("Functions for %s:\n", current_process->name);
    for (int i = 0; i < current_process->total_functions; i++)
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
    for (int i = 0; i < 50; i++)
	{
		for (int j = 1; j < (SCREEN_HEIGHT/2 + SCREEN_HEIGHT/5); j++)
		{
			scrput(i, j, ' ', VGA_COLOR_BLACK | VGA_COLOR_BLACK << 4);
		}
	}

    int function_offset = 0;
    scrprintf(0, 2, "Processes / ..");
    for (int i = 0; i < process_count; i++)
    {
        scrprintf(0, i+3+function_offset, "     - ID %d: %s\n", i, processes[i].name);
        for (int j = 0; j < processes[i].total_functions ; j++)
        {
            scrprintf(0, i+3+function_offset+j+1, "          - FN %d: %s\n", j, processes[i].functions[j].name);
        }
        function_offset += processes[i].total_functions;
    }

    int art_y = 2;
    int art_x = 24;

    scrwrite(art_x, 0+art_y, "                  .----.", VGA_COLOR_WHITE);
    scrwrite(art_x, 1+art_y, "      .---------. | == |", VGA_COLOR_WHITE);
    scrwrite(art_x, 2+art_y, "      |.-\"\"\"\"\"-.| |----|", VGA_COLOR_WHITE);
    scrwrite(art_x, 3+art_y, "      ||       || | == |", VGA_COLOR_WHITE);
    scrwrite(art_x, 4+art_y, "      ||       || |----|", VGA_COLOR_WHITE);
    scrwrite(art_x, 5+art_y, "      |'-.....-'| |::::|", VGA_COLOR_WHITE);
    scrwrite(art_x, 6+art_y, "      `\"\")---(\"\"` |___.|", VGA_COLOR_WHITE);
    scrwrite(art_x, 7+art_y, "     /:::::::::::\\\" _  \"", VGA_COLOR_WHITE);
    scrwrite(art_x, 8+art_y, "    /:::=======:::\\`\\`\\", VGA_COLOR_WHITE);
    scrwrite(art_x, 9+art_y, "   `\"\"\"\"\"\"\"\"\"\"\"\"\"`  '-'", VGA_COLOR_WHITE);

    scrprintf(0, (SCREEN_HEIGHT/2 + SCREEN_HEIGHT/5)-2, "Start a process by typing `start $id`.");
}