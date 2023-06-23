#include <syscalls.h>
#include <arch/interrupts.h>
#include <assert.h>

syscall_t syscall[255];

void add_system_call(int index, syscall_t fn)
{	
	assert(index < 255);
	syscall[index] = fn;
}

int system_call(int index, int arg1, int arg2, int arg3)
{	
	/* Call system call function based on index. */
	assert(index < 255 || index > 255);
	syscall_t fn = syscall[index];
	ENTER_CRITICAL();
	int ret = fn(arg1, arg2, arg3);
	LEAVE_CRITICAL();
	//dbgprintf("[SYSCALL] %d: %d %d %d\n", index, arg1, arg2, arg3);
	EOI(48);
	return ret;
}
