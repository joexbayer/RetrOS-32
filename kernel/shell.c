#include <shell.h>
#include <pci.h>
#include <screen.h>
#include <terminal.h>

static uint8_t SHELL_POSITION = (SCREEN_HEIGHT/2 + SCREEN_HEIGHT/5)-1;
static const uint8_t SHELL_MAX_SIZE = 25;
static uint8_t shell_column = 0;
static char shell_buffer[25];


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
	shell_column = 1;
	shell_clear();
	scrwrite(0, SHELL_POSITION, ">", VGA_COLOR_LIGHT_CYAN);
	screen_set_cursor(shell_column, SHELL_POSITION);
}

void shell_process()
{
	int width = (SCREEN_WIDTH/3)+(SCREEN_WIDTH/6)-1;
    int height = (SCREEN_HEIGHT/2 + SCREEN_HEIGHT/5)+1;
	print_pcb_status();
	while(1)
	{
		char c = kb_get_char();
		if(c == -1)
			continue;
		shell_put(c);
		print_pcb_status();
	}
}

void exec_cmd()
{

	if(strncmp("lspci", shell_buffer, strlen("lspci")))
		list_pci_devices();
	
	if(strncmp("clear", shell_buffer, strlen("clear"))){
		scr_clear();
		init_terminal();
	}
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
		exec_cmd();
		init_shell();
		return;
	}

	if(uc == backspace)
	{
		shell_column -= 1;
		scrput(shell_column, SHELL_POSITION, ' ', VGA_COLOR_WHITE);
		shell_buffer[shell_column-1] = 0;
		screen_set_cursor(shell_column-1, SHELL_POSITION);
		return;
	}

	if(shell_column == SHELL_MAX_SIZE)
	{
		return;
	}
	scrput(shell_column, SHELL_POSITION, uc, VGA_COLOR_WHITE);
	shell_buffer[shell_column-1] = uc;
	screen_set_cursor(shell_column, SHELL_POSITION);
	shell_column++;
}