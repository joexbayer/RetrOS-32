#include <scheduler.h>
#include <pcb.h>
#include <timer.h>

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
    while(1);

    CLI();
    stop_task(current_running->pid);

    _context_switch();
}

void block()
{
    current_running->running = BLOCKED;
    yield();
}

void unblock(int pid)
{
    pcb_set_running(pid);
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
            dbgprintf("[Context Switch] Running new PCB %s with page dir: %x: stack: %x kstack: %x\n", current_running->name, current_running->page_dir, current_running->esp, current_running->k_esp);
            load_page_directory(current_running->page_dir);
            //tlb_flush_addr(current_running->page_dir);
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
    load_page_directory(current_running->page_dir);
    dbgprintf("[Context Switch] Switching too PCB %s with page dir: %x, stack: %x, kstack: %x\n", current_running->name, current_running->page_dir, current_running->esp, current_running->k_esp);
    //tlb_flush_addr(current_running->page_dir);
}
