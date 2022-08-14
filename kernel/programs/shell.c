/**
 * @file shell.c
 * @author Joe Bayer (joexbayer)
 * @brief Simple program handling input from user, mainly used to handles process management.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <pci.h>
#include <keyboard.h>
#include <screen.h>
#include <terminal.h>
#include <scheduler.h>
#include <pcb.h>
#include <process.h>
#include <io.h>

#include <windowmanager.h>
#include <net/dns.h>
#include <net/icmp.h>
#include <fs/fs.h>

#include <serial.h>

#include <diskdev.h>


struct window w = {
	.x = 1,
	.y = 1,
	.height = SCREEN_HEIGHT-3,
	.width = SCREEN_WIDTH/2,
	.color = VGA_COLOR_LIGHT_BLUE,
	.visable = 1,
	.name = "TERMINAL",
	.state = {
		.column = 0,
		.row = SCREEN_HEIGHT-3,
		.color = VGA_COLOR_LIGHT_GREY
	}
	
};

#define SHELL_POSITION (get_window_height()-1)
static const uint8_t SHELL_MAX_SIZE = 50;
static uint8_t shell_column = 0;
static char shell_buffer[50];
static uint8_t shell_buffer_length = 0;

static const char newline = '\n';
static const char backspace = '\b';

/*
 *	IMPLEMENTATIONS
 */
void shell_clear()
{
	for (int i = shell_column; i < SHELL_MAX_SIZE; i++)
	{
		scrput(i, SHELL_POSITION, ' ', VGA_COLOR_WHITE);
	}	
}

void reset_shell()
{
	memset(&shell_buffer, 0, 25);
	shell_column = strlen(current_process->name)+2;
	shell_buffer_length = 0;
	scrwrite(1, SHELL_POSITION, current_process->name, VGA_COLOR_LIGHT_CYAN);
	scrwrite(shell_column, SHELL_POSITION, "> ", VGA_COLOR_LIGHT_CYAN);
	shell_column += 1;

	screen_set_cursor(shell_column, SHELL_POSITION);
	shell_clear();
}

void exec_cmd()
{
	twritef("\n");
	for (int i = 0; i < current_process->total_functions; i++)
	{
		if(strncmp(current_process->functions[i].name, shell_buffer, strlen(current_process->functions[i].name))){
			current_process->functions[i].f();
			return;
		}
	}

	if(strncmp("help", shell_buffer, strlen("help"))){
		twriteln("HELP: ");
		twriteln("ls: see processes.");
		twriteln("lsf: see functions for process.");
		twriteln("switch $id: use process functions.");
		twriteln("start $id: start a process.");
		twriteln("stop $id: stop a process.");
		return;
	}
	
	if(strncmp("lsi", shell_buffer, strlen("lsi"))){
		list_instances();
		return;
	}

	if(strncmp("lsf", shell_buffer, strlen("lsf"))){
		list_functions();
		return;
	}

	if(strncmp("switch", shell_buffer, strlen("switch"))){
		int id = atoi(shell_buffer+strlen("switch")+1);
		switch_process(id);
		return;
	}

	if(strncmp("lspci", shell_buffer, strlen("lspci"))){
		list_pci_devices();
		return;
	}

	if(strncmp("ls", shell_buffer, strlen("ls"))){
		ls("");
		return;
	}

	if(strncmp("clear", shell_buffer, strlen("clear"))){
		scr_clear();
		return;
	}

	if(strncmp("stop", shell_buffer, strlen("stop"))){
		int id = atoi(shell_buffer+strlen("stop")+1);
		stop_task(id);
		return;
	}

	if(strncmp("start", shell_buffer, strlen("start"))){
		int id = atoi(shell_buffer+strlen("stop")+1);
		start_process(id);
		return;
	}

	if(strncmp("dig", shell_buffer, strlen("dig"))){
		char* hostname = shell_buffer+strlen("dig")+1;
		hostname[strlen(hostname)-1] = 0;
		gethostname(hostname);
		return;
	}

	if(strncmp("cat", shell_buffer, strlen("cat"))){
		char* name = shell_buffer+strlen("cat")+1;
		inode_t inode = fs_open(name);

		char buf[512];
		file_read(buf, inode);
		twritef("%s\n", buf);
		file_close(inode);
		return;
	}

	if(strncmp("ping", shell_buffer, strlen("ping"))){
		char* hostname = shell_buffer+strlen("ping")+1;
		hostname[strlen(hostname)-1] = 0;
		ping(hostname);
		return;
	}

	if(strncmp("touch", shell_buffer, strlen("touch"))){
		char* hostname = shell_buffer+strlen("touch")+1;
		hostname[strlen(hostname)-1] = 0;
		create_file(hostname);
		return;
	}

	if(strncmp("ps", shell_buffer, strlen("ps"))){
		print_pcb_status();
		return;
	}

	if(strncmp("fs", shell_buffer, strlen("fs"))){
		fs_stats();
		return;
	}

	if(strncmp("fdisk", shell_buffer, strlen("fdisk"))){
		print_dev_status();
		return;
	}

	if(strncmp("sync", shell_buffer, strlen("sync"))){
		sync();
		return;
	}

	if(strncmp("exit", shell_buffer, strlen("exit"))){
		sync();
		dbgprintf("[SHUTDOWN] NETOS has shut down.\n");
		outportw(0x604, 0x2000);
		return;
	}

	if(strncmp("cd", shell_buffer, strlen("cd"))){
		char* name = shell_buffer+strlen("cd")+1;
		name[strlen(name)-1] = 0;
		chdir(name);
		return;
	}

	if(strncmp("mkdir", shell_buffer, strlen("mkdir"))){
		char* name = shell_buffer+strlen("mkdir")+1;
		name[strlen(name)-1] = 0;
		fs_mkdir(name);
		return;
	}

	if(strncmp("run", shell_buffer, strlen("run"))){
		char* name = shell_buffer+strlen("run")+1;
		create_process(name);
		return;
	}


	//twrite(shell_buffer);
}

/**
 * @brief Puts a character c into the shell line 
 * at correct position. Also detects enter.
 * 
 * @param c character to put to screen.
 */
void shell_put(char c)
{
	unsigned char uc = c;
	if(uc == newline)
	{
		shell_buffer[shell_buffer_length] = newline;
		shell_buffer_length++;
		exec_cmd();
		reset_shell();
		return;
	}

	if(uc == backspace)
	{
		if(shell_column < 1)
			return;
		shell_column -= 1;
		scrput(shell_column, SHELL_POSITION, ' ', VGA_COLOR_WHITE);
		shell_buffer[shell_buffer_length] = 0;
		shell_buffer_length--;
		screen_set_cursor(shell_column-1, SHELL_POSITION);
		return;
	}

	if(shell_column == SHELL_MAX_SIZE)
	{
		return;
	}
	scrput(shell_column, SHELL_POSITION, uc, VGA_COLOR_WHITE);
	shell_buffer[shell_buffer_length] = uc;
	shell_buffer_length++;
	screen_set_cursor(shell_column, SHELL_POSITION);
	shell_column++;
}

void shell_main()
{
	dbgprintf("Shell is running!\n");
	attach_window(&w);
	reset_shell();
	sleep(2);
	while(1)
	{
		char c = kb_get_char();
		if(c == -1)
			continue;
		shell_put(c);
	}
	
	exit();
}

PROGRAM(shell, &shell_main)
reset_shell();
PROGRAM_END