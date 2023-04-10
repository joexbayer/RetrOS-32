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
#include <pcb.h>

#define MAX_KTHREADS 64
static int total_kthreads = 0;

static struct kthread {
    char name[PCB_MAX_NAME_LENGTH];
    void (*entry)();
};

static struct kthread kthread_table[MAX_KTHREADS];

error_t register_kthread(void (*f)(), char* name)
{
    if(strlen(name)+1 > PCB_MAX_NAME_LENGTH || total_kthreads == MAX_KTHREADS)
        return -ERROR_KTHREAD_CREATE;

    kthread_table[total_kthreads].entry = f;
    memcpy(kthread_table[total_kthreads].name, name, strlen(name)+1);
    total_kthreads++;

    return 0;
}

error_t start(char* name)
{
    for (int i = 0; i < total_kthreads; i++){
        if(memcmp(name, kthread_table[i].name, strlen(kthread_table[i].name)) == 0){
            pcb_create_kthread(kthread_table[i].entry, kthread_table[i].name);
            return 0;
        }
    }

    return -ERROR_KTHREAD_START;
}

void empty();