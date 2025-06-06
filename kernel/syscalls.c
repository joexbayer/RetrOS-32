/**
 * @file syscalls.c
 * @author Joe Bayer (joexbayer)
 * @brief System calls.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <syscalls.h>
#include <screen.h>
#include <pcb.h>
#include <kutils.h>
#include <arch/interrupts.h>
#include <syscall_helper.h>
#include <assert.h>
#include <keyboard.h>

syscall_t syscall[255];

void add_system_call(int index, syscall_t fn)
{	
	assert(index < 255);
	syscall[index] = fn;
}

/**
 * @brief Function to create a userspace thread 
 * 
 * @param entry entry point of the thread
 * @param arg argument to pass to the thread
 * @param flags flags for the thread
 * @return int 
 */
int sys_create_thread(void (*entry)(), void* arg, byte_t flags)
{
	return pcb_create_thread($process->current, entry, arg, flags);
}
EXPORT_SYSCALL(SYSCALL_CREATE_THREAD, sys_create_thread);

/* Helper macros */
#define DESERIALIZE_CHAR(serialized) ((char)((serialized) >> 8))
#define DESERIALIZE_COLOR(serialized) ((char)(serialized))
int sys_screen_put(int x, int y, short packet)
{
	/* validate coordinates */
	if(x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT){
		return -1;
	}

	/* get character and color from packet */
	char c = DESERIALIZE_CHAR(packet);
	char color = DESERIALIZE_COLOR(packet);

	scrput(x, y, c, color);
	return 0;
}
EXPORT_SYSCALL(SYSCALL_SCREEN_PUT, sys_screen_put);

char sys_screen_get()
{
	return scr_keyboard_get(1);
}
EXPORT_SYSCALL(SYSCALL_SCREEN_GET, sys_screen_get);

int sys_scr_set_cursor(int x, int y)
{
	scr_set_cursor(x, y);
	return 0;
}
EXPORT_SYSCALL(SYSCALL_SET_CURSOR, sys_scr_set_cursor);

static int sys_system(const char *command)
{
	return exec_cmd((char*)command);
}
EXPORT_SYSCALL(SYSCALL_SYSTEM, sys_system);


int system_call(int index, int arg1, int arg2, int arg3)
{	
	/* Call system call function based on index. */
	if(index < 0 || index > 255){
		return -1;
	}
	
	/* the system call interrupt entered a critcal section */
	EOI(48);
	LEAVE_CRITICAL();
	syscall_t fn = syscall[index];
	$process->current->in_kernel = true;
	int ret = fn(arg1, arg2, arg3);
	$process->current->in_kernel = false;
	
	/* Enter critical section again */
	ENTER_CRITICAL();

	return ret;
}
