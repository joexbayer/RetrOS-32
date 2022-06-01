#include <shell.h>
#include <pci.h>
#include <keyboard.h>
#include <screen.h>
#include <terminal.h>
#include <pcb.h>
#include <process.h>

static uint8_t SHELL_POSITION = (SCREEN_HEIGHT)-1;
static const uint8_t SHELL_MAX_SIZE = 25;
static uint8_t shell_column = 0;
static char shell_buffer[25];
static char shell_buffer_length = 0;


/* SHELL PROTOTYPES */
void init_shell(void);
void shell_put(char c);
void shell_clear();

static const char newline = '\n';
static const char backspace = '\b';

/*
	IMPLEMENTATIONS
*/
void shell_clear()
{
	for (size_t i = shell_column; i < SHELL_MAX_SIZE; i++)
	{
		scrput(i, SHELL_POSITION, ' ', VGA_COLOR_WHITE);
	}	
}

/**
 * @brief Clears the shells buffer, screen and updates
 * cursor position.
 */
void init_shell(void)
{
	memset(&shell_buffer, 0, 25);
	if(current_process != NULL)
	{
		shell_column = strlen(current_process->name);
		shell_buffer_length = 0;
		scrwrite(0, SHELL_POSITION, current_process->name, VGA_COLOR_LIGHT_CYAN);
		scrwrite(shell_column, SHELL_POSITION, "> ", VGA_COLOR_LIGHT_CYAN);
		shell_column += 1;
	} else {
		shell_column = strlen("Shell>");
		shell_buffer_length = 0;
		scrwrite(0, SHELL_POSITION, "Shell>", VGA_COLOR_LIGHT_CYAN);
	}
	screen_set_cursor(shell_column, SHELL_POSITION);
	shell_clear();
}

void shell_process()
{
	sleep(2);
	while(1)
	{
		char c = kb_get_char();
		if(c == -1)
			continue;
		shell_put(c);
	}
}

void exec_cmd()
{
	for (size_t i = 0; i < current_process->total_functions; i++)
	{
		if(strncmp(current_process->functions[i].name, shell_buffer, strlen(current_process->functions[i].name))){
			current_process->functions[i].f();
			return;
		}
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
		list_processes();
		return;
	}

	if(strncmp("clear", shell_buffer, strlen("clear"))){
		scr_clear();
		init_terminal();
	}

	if(strncmp("stop", shell_buffer, strlen("stop"))){
		int id = atoi(shell_buffer+strlen("stop")+1);
		stop_task(id);
	}

	if(strncmp("start", shell_buffer, strlen("start"))){
		int id = atoi(shell_buffer+strlen("stop")+1);
		start_process(id);
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
		init_shell();
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