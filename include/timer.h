#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <rtc.h>

void init_timer(uint32_t frequency);
int get_time();
struct time* get_datetime();

#endif // !TIMER_H