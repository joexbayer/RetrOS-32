#include <scheduler.h>
#include <memory.h>
#include <pcb.h>
#include <timer.h>
#include <serial.h>
#include <assert.h>
#include <work.h>

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
    dbgprintf("%s (PID %d) called Exit\n", current_running->name, current_running->pid);
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
    ASSERT_CRITICAL();

    current_running = current_running->next;
    while(current_running->running != RUNNING)
    {
        switch (current_running->running)
        {
        case STOPPED:
            current_running = current_running->next;
            break;
        case ZOMBIE:
            ;
            struct pcb* old = current_running;
            current_running = current_running->next;
        	
            pcb_queue_remove_running(old);
            old->running = CLEANING;

            work_queue_add(&pcb_cleanup_routine, (void*)old->pid);
            dbgprintf("Adding work to clean up PID %d\n", old->pid);
            break;
        case NEW:
            dbgprintf("[Context Switch] Running new PCB %s (PID %d) with page dir: %x: stack: %x kstack: %x\n", current_running->name, current_running->pid, current_running->page_dir, current_running->esp, current_running->kesp);
            load_page_directory(current_running->page_dir);

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
}
