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
#include <gfx/events.h>

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
	__gfx_draw_rectangle(0, SHELL_POSITION, SHELL_HEIGHT, 8, COLOR_BLACK);
}

void reset_shell()
{
	shell_clear();
	memset(&shell_buffer, 0, SHELL_MAX_SIZE);
	shell_column = strlen(shell_name)+1;
	shell_buffer_length = 0;
	__gfx_draw_text(0, SHELL_POSITION, shell_name, COLOR_GREEN);
	shell_column += 1;
}

void ps()
{
	int ret;
	int line = 0;
	twritef("  PID  STACK       TYPE     STATE     NAME\n");
	for (int i = 0; i < MAX_NUM_OF_PCBS; i++){
		struct pcb_info info;
		ret = pcb_get_info(i, &info);
		if(ret < 0) continue;
		twritef("   %d   0x%s%x  %s  %s  %s\n", info.pid, info.is_process ? "" : "00", info.stack, info.is_process ? "process" : "kthread", pcb_status[info.state], info.name);
	}
	
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

	if(strncmp("ps", shell_buffer, strlen("ps"))){
		ps();
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
		current_running->current_directory = chdir(name);
		return;
	}

	if(strncmp("mkdir", shell_buffer, strlen("mkdir"))){
		char* name = shell_buffer+strlen("mkdir")+1;
		name[strlen(name)-1] = 0;
		fs_mkdir(name, current_running->current_directory);
		return;
	}

	if(strncmp("run", shell_buffer, strlen("run"))){
		
		dbgprintf("Command: %s\n", shell_buffer);
		/* Allocate space for 5 args */
		char** argv = (char**)kalloc(5 * sizeof(char*));
		for (int i = 0; i < 5; i++) {
			argv[i] = (char*)kalloc(100);
		}
		
		int args = parse_arguments(shell_buffer, argv);
		int pid = pcb_create_process(argv[1], args-1, &argv[1]);
		if(pid == 0)
			twritef("%s does not exist\n", argv[1]);

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
void shell_put(unsigned char c)
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
		__gfx_draw_rectangle(shell_column*8, SHELL_POSITION, 8, 8, COLOR_BLACK);
		gfx_commit();
		shell_buffer[shell_buffer_length] = 0;
		shell_buffer_length--;
		return;
	}

	if(shell_column == SHELL_MAX_SIZE)
	{
		return;
	}
	__gfx_draw_char(shell_column*8, SHELL_POSITION, uc, COLOR_WHITE);
	gfx_commit();
	shell_buffer[shell_buffer_length] = uc;
	shell_buffer_length++;
	shell_column++;
}

#include <gfx/api.h>

int c_test = 0;

void shell_main()
{
	dbgprintf("shell is running!\n");

	memset(term.textbuffer, 0, TERMINAL_BUFFER_SIZE);
	struct gfx_window* window = gfx_new_window(400, SHELL_HEIGHT);
	
	dbgprintf("shell: window 0x%x\n", window);
	__gfx_draw_rectangle(0,0, 400, SHELL_HEIGHT, 0);


	terminal_attach(&term);
	//__gfx_draw_text(0, 0, "Terminal!", VESA8_COLOR_LIGHT_GREEN);
	reset_shell();
	//sleep(2);
	while(1)
	{
		struct gfx_event event;
		gfx_event_loop(&event);

		switch (event.event)
		{
		case GFX_EVENT_KEYBOARD:
			if(event.data == -1)
				break;
			shell_put(event.data);
			c_test++;
		default:
			break;
		}
	}
	
	kernel_exit();
}