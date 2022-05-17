#include <keyboard.h>
#include <terminal.h>
#include <interrupts.h>
#include <util.h>

/*
    Basic PS/2 Keyboard driver. 
    With the help of: http://www.osdever.net/bkerndev/Docs/keyboard.htm
*/

#define KB_IRQ		33 /* Default is 1, 33 after mapped. */

static int __keyboard_presses = 0;

static void kb_callback()
{
	uint8_t scancode = inportb(0x60); /* Recieve scancode, also ACK's interrupt? */
	if(scancode & 0x80) /* Shift, alt or ctrl*/
	{
		
	}
	__keyboard_presses++;

	/* Keep track of how many keyboard presses. */
	char test[10];
	itoa(__keyboard_presses, test);
	scrwrite(10, 20, test);
}
void init_keyboard()
{
	// Firstly, register our timer callback.
	isr_install(KB_IRQ, &kb_callback);
	twrite("PS/2 Keyboard initialized.\n");
}