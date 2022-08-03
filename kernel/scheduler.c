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
    asm volatile ("movl %%eax, %%cr3 ":: "a" (current_running->page_dir));
}
