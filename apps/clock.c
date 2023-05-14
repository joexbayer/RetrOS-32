/**
 * @file clock.c
 * @author Joe Bayer (joexbayer)
 * @brief A simple clock program displaying
 * current time and date.
 * @version 0.1
 * @date 2023-02-17
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <lib/printf.h>
#include <lib/graphics.h>
#include <colors.h>
#include <rtc.h>
#include <util.h>

#define center_x(size) ((110/2) - ((size*8)/2))

static char* months[] = {"NAN", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

int main()
{   
    struct time current_time;
    gfx_create_window(110, 140);
    gfx_set_title("Clock");

    printf("Clock started\n");
    while (1)
    {
        get_current_time(&current_time);

        gfx_draw_rectangle(0, 0, 110, 140, COLOR_VGA_BG);

        int angle_id = (0.5 * (current_time.hour%12 * 60 + current_time.minute) / 6);

		gfx_draw_line(55, 55, 55 + (50*sin_60[angle_id])/1.5, 55+ (50*cos_60[angle_id])/1.5, COLOR_VGA_FG);
		gfx_draw_line(55, 55, 55 + (50*sin_60[current_time.minute])/1.1, 55+ (50*cos_60[current_time.minute])/1.1, COLOR_VGA_FG);
		gfx_draw_line(55, 55, 55 + (50*sin_60[current_time.second])/1.1, 55+ (50*cos_60[current_time.second])/1.1, COLOR_VGA_RED);

        gfx_draw_circle(55, 55, 50, COLOR_VGA_FG);

        gfx_draw_format_text(center_x(5), 112, COLOR_VGA_FG, "%s%d:%s%d", current_time.hour > 9 ? "" : "0", current_time.hour, current_time.minute > 9 ? "" : "0", current_time.minute);
        gfx_draw_format_text(center_x(6), 124, COLOR_VGA_FG, "%d. %s", current_time.day, months[current_time.month]);

        sleep(100);
    }
    
	return 0;
}