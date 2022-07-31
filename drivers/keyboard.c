/**
 * @file keyboard.c
 * @author Joe Bayer (joexbayer)
 * @brief Really simple PS/2 Keyboard driver with US keyboard layout.
 * @see http://www.osdever.net/bkerndev/Docs/keyboard.htm
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <keyboard.h>
#include <screen.h>
#include <terminal.h>
#include <interrupts.h>
#include <util.h>
#include <sync.h>

#define KB_IRQ		33 /* Default is 1, 33 after mapped. */
#define KB_BUFFER_SIZE 255

static mutex_t kb_lock;
static char kb_buffer[KB_BUFFER_SIZE];
static int kb_buffer_head = 0;
static int kb_buffer_tail = 0;

static unsigned char kbdus[128] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};
static int __keyboard_presses = 0;
static uint8_t __shift_pressed = 0;

char kb_get_char()
{
	acquire(&kb_lock);
	if(kb_buffer_tail == kb_buffer_head){
		release(&kb_lock);
		return -1;
	}
	
	char c = kb_buffer[kb_buffer_tail];
	kb_buffer_tail = (kb_buffer_tail + 1) % KB_BUFFER_SIZE;
	release(&kb_lock);
	return c;
}

void kb_add_char(char c)
{
	kb_buffer[kb_buffer_head] = c;
	kb_buffer_head = (kb_buffer_head + 1) % KB_BUFFER_SIZE;
}

static void kb_callback()
{
	uint8_t scancode = inportb(0x60); /* Recieve scancode, also ACK's interrupt? */
	switch (scancode) {
		case 0x2a: /* shift down */
			__shift_pressed = 1;
			return;
			break;
		case 0xaa: /* shift up */
			__shift_pressed = 0;
			return;
			break;

		default:
			break;
	}

	if(scancode & 0x80)
		return;

	char c = kbdus[scancode];
	kb_add_char( __shift_pressed ? c+('A'-'a') : c);
	__keyboard_presses++;

}
void init_keyboard()
{
  mutex_init(&kb_lock);
	// Firstly, register our timer callback.
	isr_install(KB_IRQ, &kb_callback);
	twrite("PS/2 Keyboard initialized.\n");
}