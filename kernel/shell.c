#include <shell.h>
#include <sync.h>
#include <screen.h>

static uint8_t SHELL_POSITION = (SCREEN_HEIGHT/2 + SCREEN_HEIGHT/5)-1;
static const uint8_t SHELL_MAX_SIZE = 25;
static uint8_t shell_column = 0;
static char shell_buffer[25];

static int shell_lock = 0; 

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
	shell_put('j');
	while(1)
	{
		char c = kb_get_char();
		if(c == -1)
			continue;
		shell_put(c);
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
		init_shell();
		// call execute command to kernel
		return;
	}

	if(uc == backspace)
	{
		shell_column -= 1;
		scrput(shell_column, SHELL_POSITION, ' ', VGA_COLOR_WHITE);
		shell_buffer[shell_column] = 0;
		screen_set_cursor(shell_column-1, SHELL_POSITION);
		return;
	}

	if(shell_column == SHELL_MAX_SIZE)
	{
		return;
	}
	scrput(shell_column, SHELL_POSITION, uc, VGA_COLOR_WHITE);
	shell_buffer[shell_column] = uc;
	screen_set_cursor(shell_column, SHELL_POSITION);
	shell_column++;
}