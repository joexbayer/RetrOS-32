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
#include <screen.h>
#include <terminal.h>
#include <interrupts.h>
#include <scheduler.h>
#include <pcb.h>

#define PIT_IRQ		32

static int tick = 0;
static int time = 0;
static int time_min = 0;
static int time_hour = 0;

static struct time current_time;

int get_time()
{
	return (time_hour*3600)+(time_min*60)+time;
}

struct time* get_datetime()
{
	return &current_time;
}

static void timer_callback()
{
	if(tick > 100){
		tick = 0;
		get_current_time(&current_time);
		time++;

		if(time > 59){
			time_min++;
			time = 0;
			if(time_min > 59){
				time_hour++;
				time_min = 0;
			}
		}
		
		scrprintf(10, 9, "%d:%d:%d", time_hour, time_min, time);
	}

	tick++;
	if(current_running != NULL)
	{
		EOI(32);
		yield();
	}
}

int time_get_difference(struct time* t1, struct time* t2)
{
	uint32_t time1 = (t1->hour*3600) + (t1->minute*60) + t1->second;
	uint32_t time2 = (t2->hour*3600) + (t2->minute*60) + t2->second;

	return time1 - time2;
}

/* http://www.jamesmolloy.co.uk/tutorial_html/5.-IRQs%20and%20the%20PIT.html */
void init_timer(uint32_t frequency)
{
	// Firstly, register our timer callback.
	isr_install(PIT_IRQ, &timer_callback);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency.
	uint32_t divisor = 1193180 / frequency;

	// Send the command byte.
	outportb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	uint8_t l = (uint8_t)(divisor & 0xFF);
	uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

	// Send the frequency divisor.
	outportb(0x40, l);
	outportb(0x40, h);

	twrite("PIT initialized.\n");
}