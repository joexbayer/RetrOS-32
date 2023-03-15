#include <scheduler.h>
#include <memory.h>
#include <pcb.h>
#include <timer.h>
#include <serial.h>
#include <assert.h>

void sleep(int time)
{
    current_running->sleep = timer_get_tick() + time;
    current_running->running = SLEEPING;
    yield();
}

void yield()
{
    //CLI();

        _context_switch();

}

void exit()
{

    current_running->running = ZOMBIE;
    _context_switch();
}

void block()
{
    current_running->running = BLOCKED;
    current_running->blocked_count++;
    _context_switch();
}

void unblock(int pid)
{
    pcb_set_running(pid);
}

void context_switch()
{   
    CLI();

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
            ;
            struct pcb* next = current_running;
            current_running = current_running->next;
            //pcb_cleanup_routine(next->pid);
            break;
        case NEW:
            dbgprintf("[Context Switch] Running new PCB %s with page dir: %x: stack: %x kstack: %x\n", current_running->name, current_running->page_dir, current_running->esp, current_running->kesp);
            load_page_directory(current_running->page_dir);
            //tlb_flush_addr(current_running->page_dir);
            STI();
            start_pcb();
            break; /* Never reached. */
        case SLEEPING:
            if(timer_get_tick() >= current_running->sleep)
                current_running->running = RUNNING;
            else
                current_running = current_running->next;
            break;
        default:
            current_running = current_running->next;
            break;
        }
    }
    //dbgprintf("[Context Switch] Switching too PCB %s with page dir: %x, stack: %x, kstack: %x\n", current_running->name, current_running->page_dir, current_running->esp, current_running->kesp);
    load_page_directory(current_running->page_dir);

    STI();

    //tlb_flush_addr(current_running->page_dir);
}
