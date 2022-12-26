/**
 * @file mouse.c
 * @author Joe Bayer (joexbayer)
 * @brief Simple PS2 Mouse controller
 * @version 0.1
 * @date 2022-12-19
 * @see https://wiki.osdev.org/PS/2_Mouse
 * @copyright Copyright (c) 2022
 * 
 */

#include <stdint.h>
#include <interrupts.h>
#include <mouse.h>
#include <io.h>
#include <serial.h>
#include <util.h>

#include <vesa.h>
#include <colors.h>
#include <gfx/composition.h>

static uint8_t mouse_cycle = 0;
static uint8_t received = 0; 
static char  mouse_byte[3];
static int32_t  mouse_x = 0;
static int32_t  mouse_y = 0;

#define MAX_MOUSE_EVENTS 15

int mouse_event_get(struct mouse* m)
{
	if(received)
		return 0;
	
	m->flags = mouse_byte[0];
	m->x = mouse_x;
	m->y = mouse_y;
	received = 1;

	return 1;
}


void mouse_handler()
{
	CLI();
   uint8_t status = inportb(MOUSE_STATUS);
	while (status & MOUSE_BBIT) {
		char mouse_in = inportb(MOUSE_PORT);
		if (status & MOUSE_F_BIT) {
			switch (mouse_cycle) {
				case 0:
					mouse_byte[0] = mouse_in;
					if (!(mouse_in & MOUSE_V_BIT)){
						return;
					}
					++mouse_cycle;
					break;
				case 1:
					mouse_byte[1] = mouse_in;
					++mouse_cycle;
					break;
				case 2:
					mouse_byte[2] = mouse_in;
					/* We now have a full mouse packet ready to use */
					if (mouse_byte[0] & 0x80 || mouse_byte[0] & 0x40) {
						/* x/y overflow? bad packet! */
						break;
					}

                    mouse_x += mouse_byte[1];
		            mouse_y -= mouse_byte[2];

                    if (mouse_x < 0) mouse_x = 0;
		            if (mouse_y < 0) mouse_y = 0;

                    if (mouse_x > 640-16) mouse_x = 640-16;
		            if (mouse_y > 480-16) mouse_y = 480-16;

					received = 0;
					mouse_cycle = 0;
					break;
			}
		}
		status = inportb(MOUSE_STATUS);
	}
}

void mouse_wait(uint8_t a_type) {
	uint32_t timeout = 100000;
	if (!a_type) {
		while (--timeout) {
			if ((inportb(MOUSE_STATUS) & MOUSE_BBIT) == 1) {
				return;
			}
		}
		return;
	} else {
		while (--timeout) {
			if (!((inportb(MOUSE_STATUS) & MOUSE_ABIT))) {
				return;
			}
		}
		return;
	}
}

inline void mouse_write(uint8_t command)
{
    mouse_wait(1);
    outportb(0x64, 0xD4);
    mouse_wait(1);
    outportb(0x60, command);
}

uint8_t mouse_read()
{
    mouse_wait(0);
    return inportb(0x60);
}

void mouse_init()
{
    dbgprintf("[MOUSE] Starting...\n");
    uint8_t _status;  //unsigned char

     mouse_wait(1);
	outportb(MOUSE_STATUS, 0xA8);
	mouse_wait(1);
	outportb(MOUSE_STATUS, 0x20);
	mouse_wait(0);
	_status = inportb(0x60) | 2;
	mouse_wait(1);
	outportb(MOUSE_STATUS, 0x60);
	mouse_wait(1);
	outportb(MOUSE_PORT, _status);
	mouse_write(0xF6);
	mouse_read();
	mouse_write(0xF4);
	mouse_read();

    isr_install(12+32, &mouse_handler);
}