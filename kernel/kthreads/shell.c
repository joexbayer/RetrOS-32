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
#include <terminal.h>
#include <scheduler.h>
#include <pcb.h>
#include <rtc.h>
#include <kthreads.h>
#include <io.h>

#include <windowmanager.h>
#include <net/dns.h>
#include <net/icmp.h>
#include <fs/fs.h>

#include <serial.h>

#include <diskdev.h>

#include <gfx/gfxlib.h>

#define SHELL_HEIGHT 275
#define SHELL_POSITION SHELL_HEIGHT-12
#define SHELL_MAX_SIZE SHELL_HEIGHT/8

static uint8_t shell_column = 0;
static char shell_buffer[SHELL_MAX_SIZE];
static uint8_t shell_buffer_length = 0;

static const char newline = '\n';
static const char backspace = '\b';

static char* shell_name = "Kernel >";

static struct terminal term  = {
	.head = 0,
	.tail = 0
};

/*
 *	IMPLEMENTATIONS
 */
void shell_clear()
{
	__gfx_draw_rectangle(0, SHELL_POSITION, SHELL_HEIGHT, 8, VESA8_COLOR_BLACK);
}

void reset_shell()
{
	shell_clear();
	memset(&shell_buffer, 0, SHELL_MAX_SIZE);
	shell_column = strlen(shell_name)+1;
	shell_buffer_length = 0;
	__gfx_draw_text(0, SHELL_POSITION, shell_name, VESA8_COLOR_LIGHT_GREEN);
	shell_column += 1;
}

void exec_cmd()
{
	twritef("Kernel > %s", shell_buffer);

	if(strncmp("lspci", shell_buffer, strlen("lspci"))){
		list_pci_devices();
		return;
	}

	if(strncmp("ls", shell_buffer, strlen("ls"))){
		ls("");
		gfx_commit();
		return;
	}

	if(strncmp("clear", shell_buffer, strlen("clear"))){
		return;
	}

	if(strncmp("unblock", shell_buffer, strlen("unblock"))){
		int id = atoi(shell_buffer+strlen("unblock")+1);
		pcb_set_running(id);
		return;
	}

	if(strncmp("dig", shell_buffer, strlen("dig"))){
		char* hostname = shell_buffer+strlen("dig")+1;
		hostname[strlen(hostname)-1] = 0;
		int ret = gethostname(hostname);
		twritef("%s IN (A) %i\n", hostname, ret);
		return;
	}

	if(strncmp("cat", shell_buffer, strlen("cat"))){
		char* name = shell_buffer+strlen("cat")+1;
		inode_t inode = fs_open(name);

		char buf[512];
		fs_read(inode, buf, 512);
		twritef("%s\n", buf);
		fs_close(inode);
		return;
	}

	if(strncmp("ping", shell_buffer, strlen("ping"))){
		char* hostname = shell_buffer+strlen("ping")+1;
		hostname[strlen(hostname)-1] = 0;
		ping(hostname);
		return;
	}

	if(strncmp("touch", shell_buffer, strlen("touch"))){
		char* filename = shell_buffer+strlen("touch")+1;
		filename[strlen(filename)-1] = 0;
		fs_create(filename);
		return;
	}

	if(strncmp("ps", shell_buffer, strlen("ps"))){
		print_pcb_status();
		return;
	}
	
	if(strncmp("netinfo", shell_buffer, strlen("netinfo"))){
		networking_print_status();
		return;
	}

	if(strncmp("sync", shell_buffer, strlen("sync"))){
		sync();
		current_running->gfx_window->width = 400;
		current_running->gfx_window->height = 400;
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
		name[strlen(name)-1] = 0;
		int pid = pcb_create_process(name);
		if(pid == 0)
			twritef("%s does not exist\n", name);

		return;
	}
	int r = start(shell_buffer);
	if(r == -1)
		twritef("Unknown command: %s\n", shell_buffer);
	else
		twriteln("Started process.");

	gfx_commit();
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
		terminal_commit();
		reset_shell();
		
		return;
	}

	if(uc == backspace)
	{
		if(shell_buffer_length < 1)
			return;
		shell_column -= 1;
		__gfx_draw_rectangle(shell_column*8, SHELL_POSITION, 8, 8, VESA8_COLOR_BLACK);
		gfx_commit();
		shell_buffer[shell_buffer_length] = 0;
		shell_buffer_length--;
		return;
	}

	if(shell_column == SHELL_MAX_SIZE)
	{
		return;
	}
	__gfx_draw_char(shell_column*8, SHELL_POSITION, uc, VESA8_COLOR_WHITE);
	gfx_commit();
	shell_buffer[shell_buffer_length] = uc;
	shell_buffer_length++;
	shell_column++;
}

#include <gfx/api.h>

int c_test = 0;

void shell_main()
{
	dbgprintf("Shell is running!\n");

	memset(term.textbuffer, 0, TERMINAL_BUFFER_SIZE);
	struct gfx_window* window = gfx_new_window(400, SHELL_HEIGHT);

	dbgprintf("Shell: window 0x%x\n", window);
	__gfx_draw_rectangle(0,0, 400, SHELL_HEIGHT, 0);


	terminal_attach(&term);
	//__gfx_draw_text(0, 0, "Terminal!", VESA8_COLOR_LIGHT_GREEN);
	reset_shell();
	//sleep(2);
	while(1)
	{
		char c = kb_get_char();
		if(c == -1)
			continue;
		shell_put(c);
		c_test++;
	}
	
	exit();
}