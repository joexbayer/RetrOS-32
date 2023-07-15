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
#include <kthreads.h>
#include <terminal.h>
#include <scheduler.h>
#include <pcb.h>
#include <assert.h>
#include <kutils.h>
#include <errors.h>

#define MAX_KTHREADS 64
static int total_kthreads = 0;

static struct kthread {
    char name[PCB_MAX_NAME_LENGTH];
    void (*entry)();
} kthread_table[MAX_KTHREADS];

/**
 * @brief Kernel thread entry function.
 * Calls the entry function of the kthread.
 * Exits the thread when the entry function returns.
 * @param argc number of arguments
 * @param args arguments
 */
void __noreturn kthread_entry(int argc, char* args[])
{   
    assert(!current_running->is_process);

    void (*entry)() = current_running->thread_eip;
    entry();
    kernel_exit();
    UNREACHABLE();
}

/**
 * @brief Creates a kernel thread.
 * Registers the kthread entry function in the kthead table.
 * @param f entry function
 * @param name name of the kthread 
 * @return error_t  ERROR_OK on success, else error code
 */
error_t register_kthread(void (*f)(), char* name)
{
    if(strlen(name)+1 > PCB_MAX_NAME_LENGTH || total_kthreads == MAX_KTHREADS)
        return -ERROR_KTHREAD_CREATE;

    kthread_table[total_kthreads].entry = f;
    memcpy(kthread_table[total_kthreads].name, name, strlen(name)+1);
    total_kthreads++;

    return ERROR_OK;
}

/**
 * @brief Starts a kernel thread.
 * Creates a PCB for the kthread and adds it to the scheduler.
 * @param name name of the kthread
 * @return error_t ERROR_OK on success, else error code
 */
error_t start(char* name)
{
    for (int i = 0; i < total_kthreads; i++){
        if(memcmp(name, kthread_table[i].name, strlen(kthread_table[i].name)) == 0){
            int pid = pcb_create_kthread(kthread_table[i].entry, kthread_table[i].name);
            return pid;
        }
    }

    return -ERROR_KTHREAD_START;
}

void empty();