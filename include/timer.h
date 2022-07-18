#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <rtc.h>

#define TIME_TO_INT(time) (((time)->hour*3600) + ((time)->minute*60) + (time)->second)


void init_timer(uint32_t frequency);
int get_time();
struct time* get_datetime();

int time_get_difference();

#endif // !TIMER_H