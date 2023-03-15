#include <scheduler.h>
#include <memory.h>
#include <pcb.h>
#include <timer.h>
#include <serial.h>
#include <assert.h>

void kernel_sleep(int time)
{
    current_running->sleep = timer_get_tick() + time;
    current_running->running = SLEEPING;
    kernel_yield();
}

void kernel_yield()
{
    _context_switch();
}

void kernel_exit()
{
    current_running->running = ZOMBIE;
    _context_switch();
}

void kernel_block()
{
    current_running->running = BLOCKED;
    current_running->blocked_count++;
    _context_switch();
}

void kernel_unblock(int pid)
{
    pcb_set_running(pid);
}

void context_switch()
{   
    CLI();
    struct pcb* next;

    current_running = current_running->next;
    if(current_running == NULL)
        current_running = pcb_get_new_running();
    
    while(current_running->running != RUNNING)
    {
        switch (current_running->running)
        {
        case STOPPED:
            current_running = current_running->next;
            break;

        case ZOMBIE:
            next = current_running;
            current_running = current_running->next;
            //pcb_cleanup_routine(next->pid);
            break;

        case NEW:
            dbgprintf("[Context Switch] Running new PCB %s with page dir: 0x%x: stack: 0x%x kstack: 0x%x\n", current_running->name, current_running->page_dir, current_running->esp, current_running->kesp);
            load_page_directory(current_running->page_dir);
            STI();
            start_pcb();
            UNREACHABLE();
    
        case SLEEPING:
            if(timer_get_tick() >= current_running->sleep){
                current_running->running = RUNNING;
                break;
            }
        default:
            current_running = current_running->next;
            break;
        }
    }
    //dbgprintf("[Context Switch] Switching too PCB %s with page dir: %x, stack: %x, kstack: %x\n", current_running->name, current_running->page_dir, current_running->esp, current_running->kesp);
    load_page_directory(current_running->page_dir);

    STI();
}
