/**
 * @file terminal.c
 * @author Joe Bayer (joexbayer)
 * @brief Handles terminal input and currently.. UI drawing.
 * @version 0.2
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <terminal.h>
#include <args.h>
#include <colors.h>
#include <gfx/gfxlib.h>
#include <pcb.h>
#include <terminal.h>
#include <gfx/theme.h>

#define MAX_FMT_STR_SIZE 50
/* OLD */

void terminal_set_color(color_t color)
{
	current_running->term->text_color = color;
}

static void __terminal_syntax(unsigned char c)
{
	struct gfx_theme* theme = kernel_gfx_current_theme();
	/* Set different colors for different syntax elements */
	switch (c) {
		case '>':
		case '/':
		case '\\':
			/* Highlight preprocessor directives */
			terminal_set_color(COLOR_VGA_MISC);
			break;
		case '"':
		case ':':
		case '-':
			/* Highlight string literals and character literals */
			terminal_set_color(COLOR_VGA_GREEN);
			break;
		default:
			terminal_set_color(theme->terminal.text);
			break;
	}
}

void terminal_attach(struct terminal* term)
{
	current_running->term = term;
	term->screen = current_running->gfx_window;
}

static int __next_newline(void* _data)
{
	char* data = (char*) _data;

	while (*data != '\n'){
		data++;
	}
	data++;

	return (int)data - (int)_data;
}

static void __terminal_remove_line(struct terminal* term)
{
	int skip = __next_newline(term->textbuffer);
	memcpy(term->textbuffer, &term->textbuffer[skip], term->head-skip);
	term->head -= skip;
}

static void __terminal_scroll(struct terminal* term)
{
	/*if(term->tail == term->head)
		return;
		
	while(term->textbuffer[term->tail] != '\n' && term->head != term->tail){
		term->tail++;
	}
	term->tail++;*/

	__terminal_remove_line(term);
}

void terminal_commit()
{
	current_running->term->ops->commit(current_running->term);
}

void terminal_putchar(char c)
{
	return;
	current_running->term->ops->putchar(current_running->term, c);
}

/* terminal prototypes */
static int __terminal_write(struct terminal* term, const char* data, int size);
static int __terminal_writef(struct terminal* term, char* fmt, ...);
static int __terminal_putchar_graphics(struct terminal* term, char c);
static int __terminal_putchar_textmode(struct terminal* term, char c);
static int __terminal_commit_textmode(struct terminal* term);
static int __terminal_commit_graphics(struct terminal* term);
static int __terminal_attach(struct terminal* term);
static int __terminal_detach(struct terminal* term);
static int __terminal_reset(struct terminal* term);

/* default terminal ops (graphics) */
struct terminal_ops terminal_ops = {
	.write = __terminal_write,
	.writef = __terminal_writef,
	.putchar = __terminal_putchar_graphics,
	.commit = __terminal_commit_graphics,
	.attach = __terminal_attach,
	.detach = __terminal_detach,
	.reset = __terminal_reset
};

/**
 * Creates a new terminal.
 * Attaches terminal ops to the terminal.
 * @return void
 */
struct terminal* terminal_create()
{
	struct terminal* term = (struct terminal*) malloc(sizeof(struct terminal));
	if(term == NULL) return NULL;

	memset(term->textbuffer, 0, TERMINAL_BUFFER_SIZE);
	term->ops = &terminal_ops;
	term->head = 0;
	term->tail = 0;
	term->lines = 0;
	term->text_color = COLOR_WHITE;
	term->bg_color = COLOR_BLACK;

	kref_init(&term->ref);

	return term;
}

/**
 * Destroys a terminal.
 * @param struct terminal* term
 * @return int 0 on success, -1 on failure
 */
int terminal_destroy(struct terminal* term)
{
	if(term == NULL) return -1;

	if(term->ref.refs > 0){
		return 0;
	}

	free(term);
	return 0;
}

/* Terminal operations implementations */

static int __terminal_reset(struct terminal* term)
{
	if(term == NULL) return -1;

	term->head = 0;
	term->tail = 0;
	term->lines = 0;

	memset(term->textbuffer, 0, TERMINAL_BUFFER_SIZE);

	return 0;
}

static int __terminal_putchar_graphics(struct terminal* term, char c)
{
	if(term == NULL) return -1;

	if(c == '\n'){
		if((term->screen->inner_height/8) -1 == term->lines){
			__terminal_scroll(term);
		} else {
			term->lines++;
		}

		/* should be a flush syscall */
		if(term->screen != NULL && term->screen != current_running->gfx_window){
			term->ops->commit(term);
		}
	}

	term->textbuffer[term->head] = c;
	term->head++;

	serial_put(c);

	if(term->screen != NULL && term->screen != current_running->gfx_window){
		term->screen->changed = 1;
	} else {
		gfx_commit();
	}

	return 1;
}

static int __terminal_putchar_textmode(struct terminal* term, char c)
{
	if(term == NULL) return -1;

	return 1;
}

static int __terminal_write(struct terminal* term, const char* data, int size)
{
	if(term == NULL) return -1;

	for (int i = 0; i < size; i++){
		term->ops->putchar(term, data[i]);
	}

	return size;
}

static int __terminal_attach(struct terminal* term)
{
	if(term == NULL) return -1;

	/* This is a BAD solution, FIXME Badly */
	current_running->term = term;
	if(term->screen == NULL){
		term->screen = current_running->gfx_window;
	}

	kref_get(&term->ref);
	
	return 0;
} 

static int __terminal_detach(struct terminal* term)
{
	if(term == NULL) return -1;

	current_running->term = NULL;
	term->screen = NULL;

	kref_put(&term->ref);

	return 0;
}

static int __terminal_commit_textmode(struct terminal* term)
{
	if(term == NULL) return -1;

	return 0;
}

static int __terminal_commit_graphics(struct terminal* term)
{
	if(term == NULL) return -1;

	struct gfx_theme* theme = kernel_gfx_current_theme();
	int x = 0, y = 0;
	kernel_gfx_draw_rectangle(term->screen, 0, 0, term->screen->inner_width, term->screen->inner_height, theme->terminal.background);
	for (int i = term->tail; i < term->head; i++){
		if(term->textbuffer[i] == '\n'){
			x = 0;
			y++;
			continue;
		}

		__terminal_syntax(term->textbuffer[i]);
		kernel_gfx_draw_char(term->screen, 1 + x*8, 1+ y*8, term->textbuffer[i], term->text_color);
		x++;
	}

	return 0;
}

static int __terminal_writef(struct terminal* term, char* fmt, ...)
{
	if(term == NULL) return -1;

	va_list args;

	int x_offset = 0;
	int written = 0;
	char str[MAX_FMT_STR_SIZE];
	int num = 0;

	va_start(args, fmt);

	while (*fmt != '\0') {
		switch (*fmt){
			case '%':
				memset(str, 0, MAX_FMT_STR_SIZE);
				switch (*(fmt+1))
				{
					case 'd': ;
						num = va_arg(args, int);
						itoa(num, str);
						term->ops->write(term, str, strlen(str));
						x_offset += strlen(str);
						break;
					case 'i': ;
						num = va_arg(args, int);
						unsigned char bytes[4];
						bytes[0] = (num >> 24) & 0xFF;
						bytes[1] = (num >> 16) & 0xFF;
						bytes[2] = (num >> 8) & 0xFF;
						bytes[3] = num & 0xFF;
						term->ops->writef(term, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
						break;
					case 'p': ; /* p for padded int */
						num = va_arg(args, int);
						itoa(num, str);

						if(strlen(str) < 5){
							int pad = 5-strlen(str);
							for (int i = 0; i < pad; i++){
								term->ops->putchar(term, '0');
							}
						}

						term->ops->write(term, str, strlen(str));
						x_offset += strlen(str);
						break;
					case 'x':
					case 'X': ;
						num = va_arg(args, int);
						itohex(num, str);
						term->ops->write(term, str, strlen(str));
						x_offset += strlen(str);
						break;
					case 's': ;
						char* str_arg = va_arg(args, char *);
						term->ops->write(term, str_arg, strlen(str_arg));
						x_offset += strlen(str_arg);
						break;
					case 'c': ;
						char char_arg = (char)va_arg(args, int);
						term->ops->putchar(term, char_arg);
						x_offset++;
						break;
					default:
						term->ops->putchar(term, *fmt);
						x_offset++;
						break;
				}
				fmt++;
				break;
			default:  
				term->ops->putchar(term, *fmt);
				x_offset++;
			}
        fmt++;
    }
	written += x_offset;
	return written;
}


