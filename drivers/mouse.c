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
#include <arch/interrupts.h>
#include <mouse.h>
#include <arch/io.h>
#include <serial.h>
#include <libc.h>
#include <kutils.h>

#include <vbe.h>
#include <colors.h>
#include <gfx/composition.h>

static struct ps2_mouse mouse_device = {
	.packet = {
		.flags = 0,
		.x = 0,
		.y = 0
	},
	.x = 0,
	.y = 0,
	.received = 0,
	.initilized = 0,
	.cycle = 0
};

#define MAX_MOUSE_EVENTS 15

/* static helper functions wait / write /read */

/* wait for mouse to be ready */
static void mouse_wait(unsigned char a_type) {
	unsigned int timeout = 100000;
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

/* write a byte to mouse */
static void mouse_write(uint8_t command)
{
    mouse_wait(1);
    outportb(0x64, 0xD4);
    mouse_wait(1);
    outportb(0x60, command);
}

/* read a byte from mouse */
static uint8_t mouse_read()
{
    mouse_wait(0);
    return inportb(0x60);
}

/* main API to get a mouse event */
int mouse_get_event(struct mouse* m)
{
	if(mouse_device.received == 1)
		return 0;
	
	struct mouse event = {
		.flags = mouse_device.packet.flags,
		.x = mouse_device.x,
		.y = mouse_device.y
	};
	
	*m = event;

	mouse_device.received = 1;

	return 1;
}

void __int_handler __mouse_handler()
{
   	uint8_t status = inportb(MOUSE_STATUS);
	while (status & MOUSE_BBIT) {
		char mouse_in = inportb(MOUSE_PORT);
		if (status & MOUSE_F_BIT) {
			switch (mouse_device.cycle) {
				case 0:{
						mouse_device.packet.flags = mouse_in;
						if (!(mouse_in & MOUSE_V_BIT)){
							return;
						}
						++mouse_device.cycle;
					}
					break;
				case 1:{
						mouse_device.packet.x = mouse_in;
						++mouse_device.cycle;
					}
					break;
				case 2:{
						mouse_device.packet.y = mouse_in;
						/* We now have a full mouse packet ready to use */
						if (mouse_device.packet.flags & MOUSE_Y_OVERFLOW || mouse_device.packet.flags & MOUSE_X_OVERFLOW) {
							/* x/y overflow? bad packet! */
							mouse_device.cycle = 0;
							break;
						}

						mouse_device.x += mouse_device.packet.x;
						mouse_device.y -= mouse_device.packet.y;

						if (mouse_device.x < 0) mouse_device.x = 0;
						if (mouse_device.y < 0) mouse_device.y = 0;

						if (mouse_device.x > vbe_info->width-16) mouse_device.x = vbe_info->width-16;
						if (mouse_device.y > vbe_info->height-16) mouse_device.y = vbe_info->height-16;

						mouse_device.received = 0;
						mouse_device.cycle = 0;
					}
					break;
			}
		}
		status = inportb(MOUSE_STATUS);
	}
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

    interrupt_install_handler(12+32, &__mouse_handler);
}