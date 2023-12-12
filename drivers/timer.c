/**
 * @file timer.c
 * @author Joe Bayer (joexbayer)
 * @brief PIT driver, used for preemptive scheduling and timing.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <timer.h>
#include <serial.h>
#include <arch/interrupts.h>
#include <scheduler.h>
#include <pcb.h>
#include <arch/io.h>
#include <kutils.h>

#define PIT_IRQ		32

static unsigned long tick = 0;
static void __int_handler timer_callback()
{
	tick++;
	current_running->preempts++;
	EOI(32);
	if(current_running != NULL)
	{
		kernel_yield();
	}
}

int timer_get_tick()
{
	return tick;
}

int time_get_difference(struct time* t1, struct time* t2)
{
	uint32_t time1 = (t1->hour*3600) + (t1->minute*60) + t1->second;
	uint32_t time2 = (t2->hour*3600) + (t2->minute*60) + t2->second;

	return time1 - time2;
}

/* http://www.jamesmolloy.co.uk/tutorial_html/5.-IRQs%20and%20the%20PIT.html */
void init_pit(uint32_t frequency)
{
	/* Firstly, register our timer callback. */
	interrupt_install_handler(PIT_IRQ, &timer_callback);

	/* The value we send to the PIT is the value to divide it's input clock */
	/* (1193180 Hz) by, to get our required frequency. */
	
	uint32_t divisor = (1193180/2) / frequency;
	//uint32_t divisor = (1193180) / frequency;

	/* Send the command byte. */
	outportb(0x43, 0x36);

	/* Divisor has to be sent byte-wise, so split here into upper/lower bytes. */
	uint8_t l = (uint8_t)(divisor & 0xFF);
	uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

	/* Send the frequency divisor. */
	outportb(0x40, l);
	outportb(0x40, h);

	dbgprintf("PIT initialized.\n");
}