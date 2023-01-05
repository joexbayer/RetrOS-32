/**
 * @file rtc.c
 * @author Joe Bayer (joexbayer)
 * @brief RTC "driver" for getting currentime and date.
 * @see https://wiki.osdev.org/CMOS
 * @version 0.1
 * @date 2022-06-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <rtc.h>
#include <io.h>

enum {
    RTC_ADDRESS = 0x70,
    RTC_DATA    = 0x71
};

int __rtc_check_in_progress()
{
      outportb(RTC_ADDRESS, 0x0A);
      return (inportb(RTC_DATA) & 0x80);
}

uint8_t __rtc_get_register(int reg)
{
    outportb(RTC_ADDRESS, reg);
    return inportb(RTC_DATA);
}


int get_current_time(struct time* time)
{
    uint8_t reg_b;

    while (__rtc_check_in_progress());
    time->second = __rtc_get_register(0x00);
    time->minute = __rtc_get_register(0x02); 
    time->hour = __rtc_get_register(0x04)+2; 
    time->day = __rtc_get_register(0x07); 
    time->month = __rtc_get_register(0x08); 
    time->year = __rtc_get_register(0x09); 

    reg_b = __rtc_get_register(0x0B);

    if (!(reg_b & 0x04))
    {
        time->second = (time->second & 0x0F) + ((time->second / 16) * 10);
        time->minute = (time->minute & 0x0F) + ((time->minute / 16) * 10);
        time->hour = (( (time->hour & 0x0F) + (((time->hour & 0x70) / 16) * 10) ) | (time->hour & 0x80)) - 1;
        time->day = (time->day & 0x0F) + ((time->day / 16) * 10);
        time->month = (time->month & 0x0F) + ((time->month / 16) * 10);
        time->year = (time->year & 0x0F) + ((time->year / 16) * 10);
    }

    if (!(reg_b & 0x02) && (time->hour & 0x80)) {
            time->hour = ((time->hour & 0x7F) + 12) % 24;
      }

    return 1;
}

int get_time()
{
    struct time t;
    get_current_time(&t);
	return (t.hour*3600)+(t.minute*60)+t.second;
}