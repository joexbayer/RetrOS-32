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
#include <gfx/events.h>
#include <conf.h>

#include <screen.h>

#define MAX_FMT_STR_SIZE 50
/* OLD */

int scan(ubyte_t* data, int size)
{
	if($process->current == NULL || $process->current->term == NULL) return -1;

	return $process->current->term->ops->scan($process->current->term, data, size);
}

void terminal_set_color(color_t color)
{
	$process->current->term->text_color = color;
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
			terminal_set_color($process->current->term->org_text_color);
			break;
	}
}

void terminal_attach(struct terminal* term)
{
	$process->current->term = term;
	term->screen = $process->current->gfx_window;
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
	$process->current->term->ops->commit($process->current->term);
}

void terminal_putchar(char c)
{
	if($process->current == NULL || $process->current->term == NULL) return;
	
	$process->current->term->ops->putchar($process->current->term, c);
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
static int __terminal_set_ops(struct terminal* term, struct terminal_ops* ops);
static int __terminal_scan_graphics(struct terminal* term, ubyte_t* data, int size);	
static int __terminal_scan_textmode(struct terminal* term, ubyte_t* data, int size);
static int __terminal_scanf(struct terminal* term, char* fmt, ...);
static int __terminal_getchar_graphics(struct terminal* term);
static int __terminal_getchar_textmode(struct terminal* term);

/* default terminal ops (graphics) */
struct terminal_ops terminal_ops = {
	.write = __terminal_write,
	.writef = __terminal_writef,
	.putchar = __terminal_putchar_graphics,
	.commit = __terminal_commit_graphics,
	.attach = __terminal_attach,
	.detach = __terminal_detach,
	.reset = __terminal_reset,
	.set = __terminal_set_ops,
	.scan = __terminal_scan_graphics,
	.scanf = __terminal_scanf,
	.getchar = __terminal_getchar_graphics,
};

/**
 * Creates a new terminal.
 * Attaches terminal ops to the terminal.
 * @return void
 */
struct terminal* terminal_create(terminal_flags_t flags)
{
	struct terminal* term = create(struct terminal);
	if(term == NULL) return NULL;

	term->textbuffer = (char*) kalloc(1024);
	if(term->textbuffer == NULL){
		kfree(term);
		return NULL;
	}

	term->ops = &terminal_ops;

	term->head = 0;
	term->tail = 0;
	term->lines = 0;
	term->text_color =  0x1c;
	term->org_text_color = term->text_color;
	term->bg_color = COLOR_BLACK;
	term->screen = NULL;

	char* bg_color = config_get_value("terminal", "background");
	if(bg_color != NULL){
		term->bg_color = htoi(bg_color);
	}

	char* text_color = config_get_value("terminal", "text");
	if(text_color != NULL){
		term->text_color = htoi(text_color);
		term->org_text_color = term->text_color;
	}

	if(HAS_FLAG(flags, TERMINAL_TEXT_MODE)){
		term->ops->putchar = __terminal_putchar_textmode;
		term->ops->commit = __terminal_commit_textmode;
		term->ops->scan = __terminal_scan_textmode;
		term->ops->getchar = __terminal_getchar_textmode;

		term->bg_color = VGA_COLOR_BLUE;
		term->text_color = VGA_COLOR_WHITE;
	}

	$process->current->term = term;
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

	kfree(term);
	return 0;
}

/* Terminal operations implementations */

static int __terminal_set_ops(struct terminal* term, struct terminal_ops* ops)
{
	if(term == NULL) return -1;

	/* overwrite ops that are not 0 */
	if(ops->write == 0) ops->write = term->ops->write;
	if(ops->writef == 0) ops->writef = term->ops->writef;
	if(ops->putchar == 0) ops->putchar = term->ops->putchar;
	if(ops->commit == 0) ops->commit = term->ops->commit;
	if(ops->attach == 0) ops->attach = term->ops->attach;
	if(ops->detach == 0) ops->detach = term->ops->detach;
	if(ops->reset == 0) ops->reset = term->ops->reset;
	if(ops->scan == 0) ops->scan = term->ops->scan;
	if(ops->scanf == 0) ops->scanf = term->ops->scanf;
	if(ops->getchar == 0) ops->getchar = term->ops->getchar;
	
	term->ops = ops;

	return 0;
}

static int __terminal_getchar_graphics(struct terminal* term)
{
	if(term == NULL) return -1;

	struct gfx_event event;
	gfx_event_loop(&event, GFX_EVENT_BLOCKING);

	switch (event.event){
	case GFX_EVENT_KEYBOARD:
		return event.data;
	case GFX_EVENT_EXIT:
		kernel_exit();
		return -1;
	default:
		break;
	}

	return -1;
}

static int __terminal_getchar_textmode(struct terminal* term)
{
	if(term == NULL) return -1;

	ubyte_t c = kb_get_char();
	if(c == 0) return -1;

	return c;
}


static int __terminal_scan_textmode(struct terminal* term, ubyte_t* data, int size)
{
	if(term == NULL || data == NULL) return -1;

	int i = 0;
	ubyte_t c = 0;
	while (i < size && c != '\n'){
		c = kb_get_char();
		if(c == 0) continue;

		if(c == CTRLC){
			return -1;
		}

		if(c == '\n') {
			break;
		}

		if(c == '\b'){
			data[i] = 0;
			i--;
			if(i < 0) i = 0;
			term->head--;
			term->textbuffer[term->head] = 0;
			term->ops->commit(term);
			continue;
		}

		data[i] = c;
		i++;

		term->ops->putchar(term, c);
		term->ops->commit(term);
	}

	data[i] = '\0';

	return i;
}

static int __terminal_scan_graphics(struct terminal* term, ubyte_t* data, int size)
{
	if(term == NULL || data == NULL) return -1;

	int i = 0;
	ubyte_t c = 0;
	while (i < size && c != '\n'){
		struct gfx_event event;
		gfx_event_loop(&event, GFX_EVENT_BLOCKING);

		switch (event.event){
		case GFX_EVENT_KEYBOARD:
			switch (event.data){
			case CTRLC:
				return -1;
			default:{
					c = event.data;

					if(c == '\b'){
						data[i] = 0;
						i--;
						if(i < 0) i = 0;
						term->head--;
						term->textbuffer[term->head] = 0;
						term->ops->commit(term);
						break;
					}

					data[i] = c;
					i++;

					term->ops->putchar(term, c);
					term->ops->commit(term);
				}
				break;
			}
			break;
		case GFX_EVENT_EXIT:
			kernel_exit();
			return -1;
		default:
			break;
		}
	}

	data[i] = '\0';

	return i;
}

static int __terminal_scanf(struct terminal* term, char* fmt, ...)
{
	return -1;
}

static int __terminal_reset(struct terminal* term)
{
	if(term == NULL) return -1;

	term->head = 0;
	term->tail = 0;
	term->lines = 0;

	memset(term->textbuffer, 0, 1024);

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
		if(term->screen != NULL && term->screen != $process->current->gfx_window){
			term->ops->commit(term);
		}
	}
	term->textbuffer[term->head] = c;
	term->head++;

	serial_put(c);

	if(term->screen != NULL && term->screen != $process->current->gfx_window){
		term->screen->changed = 1;
	} else {
		gfx_commit();
	}

	return 1;
}

static int __terminal_putchar_textmode(struct terminal* term, char c)
{
	if(term == NULL) return -1;

	if(c == '\n'){
		if(SCREEN_HEIGHT-2 <= term->lines){
			__terminal_scroll(term);
		} else {
			term->lines++;
		}
	}

	term->textbuffer[term->head] = c;
	term->head++;

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
	$process->current->term = term;
	if(term->screen == NULL){
		term->screen = $process->current->gfx_window;
	}

	kref_get(&term->ref);
	
	return 0;
} 

static int __terminal_detach(struct terminal* term)
{
	if(term == NULL) return -1;

	$process->current->term = NULL;
	term->screen = NULL;

	kref_put(&term->ref);

	return 0;
}

static int __terminal_commit_textmode(struct terminal* term)
{
	if(term == NULL) return -1;

	/* Clear screen */
	for (int i = 1; i < SCREEN_HEIGHT-1; i++){
		for (int j = 1; j < SCREEN_WIDTH-2; j++){
			scrput(j, i, ' ', term->text_color | term->bg_color << 4);
		}
	}

	int x = 0, y = 1;
	for (int i = term->tail; i < term->head; i++){
		if(term->textbuffer[i] == '\n'){
			x = 0;
			y++;
			continue;
		}

		scrput(1+x, y, term->textbuffer[i], term->text_color | term->bg_color << 4);
		x++;
	}

	return 0;
}

static int __terminal_commit_graphics(struct terminal* term)
{
	if(term == NULL || term->screen == NULL) return -1;

	struct gfx_theme* theme = kernel_gfx_current_theme();
	int x = 0, y = 0;
	kernel_gfx_draw_rectangle(term->screen, 0, 0, term->screen->inner_width, term->screen->inner_height, term->bg_color);
	for (int i = term->tail; i < term->head; i++){
		if(term->textbuffer[i] == '\n'){
			x = 0;
			y++;
			continue;
		}

		__terminal_syntax(term->textbuffer[i]);
		kernel_gfx_draw_char(term->screen, 1 + x*8, 2+ y*8, term->textbuffer[i], term->text_color);
		x++;
	}

	term->screen->changed = 1;

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
	int padding = 0;

	va_start(args, fmt);

	while (*fmt != '\0') {
		switch (*fmt){
			case '%':
				memset(str, 0, MAX_FMT_STR_SIZE);
				
				/* Check if the format specifier is a digit (for padding) */
				padding = 0;
				if (*(fmt+1) >= '0' && *(fmt+1) <= '9') {
					padding = *(fmt+1) - '0';
					fmt++;
				}

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
						int len = strlen(str_arg);
						term->ops->write(term, str_arg, len);
						x_offset += strlen(str_arg);

						/* Pad the string if needed */
						if (padding > len) {
							for (int i = 0; i < padding - len; i++) {
								term->ops->putchar(term, ' ');
								x_offset++;
							}
						}

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


