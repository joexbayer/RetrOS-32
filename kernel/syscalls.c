#include <syscalls.h>
#include <pcb.h>
#include <kutils.h>
#include <arch/interrupts.h>
#include <syscall_helper.h>
#include <assert.h>

syscall_t syscall[255];

void add_system_call(int index, syscall_t fn)
{	
	assert(index < 255);
	syscall[index] = fn;
}

int sys_create_thread(void* entry)
{
	return pcb_create_thread(current_running, entry, "thread", 0);
}
EXPORT_SYSCALL(SYSCALL_CREATE_THREAD, sys_create_thread);

int system_call(int index, int arg1, int arg2, int arg3)
{	
	/* the system call interrupt entered a critcal section */
	LEAVE_CRITICAL();
	
	/* Call system call function based on index. */
	if(index < 0 || index > 255){
		return -1;
	}

	syscall_t fn = syscall[index];
	int ret = fn(arg1, arg2, arg3);
	EOI(48);

	/* Enter critical section again */
	ENTER_CRITICAL();

	return ret;
}
