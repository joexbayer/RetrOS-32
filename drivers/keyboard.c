#include <keyboard.h>
#include <screen.h>
#include <terminal.h>
#include <interrupts.h>
#include <util.h>

/*
    Basic PS/2 Keyboard driver. 
    With the help of: http://www.osdever.net/bkerndev/Docs/keyboard.htm
*/

#define KB_IRQ		33 /* Default is 1, 33 after mapped. */

unsigned char kbdus[128] =
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

static void kb_callback()
{
	uint8_t scancode = inportb(0x60); /* Recieve scancode, also ACK's interrupt? */
	if(scancode & 0x80) /* Shift, alt or ctrl*/
	{
		
	}
	else
	{
		shell_put(kbdus[scancode]);
	}
	__keyboard_presses++;

	/* Keep track of how many keyboard presses. */
	char test[10];
	itoa(__keyboard_presses, test);
	scrwrite(10, 20, test, VGA_COLOR_WHITE);
}
void init_keyboard()
{
	// Firstly, register our timer callback.
	isr_install(KB_IRQ, &kb_callback);
	twrite("PS/2 Keyboard initialized.\n");
}