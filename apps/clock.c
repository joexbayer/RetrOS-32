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
#include <gfx/events.h>
#include <colors.h>
#include <rtc.h>
#include <libc.h>
#include <syscall_helper.h>

#define center_x(size) ((110/2) - ((size*8)/2))

static char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

static volatile int shared;

int main()
{   
    struct time now;
    struct gfx_event event;
    gfx_create_window(110, 140, 0);
    gfx_set_title("Time & Date");

    printf("Clock started\n");
    while (1){
        get_current_time(&now);

        gfx_draw_rectangle(0, 0, 110, 140, 30);

        int angle_id = (0.5 * (now.hour%12 * 60 + now.minute) / 6);
        gfx_draw_circle(55, 55, 50, 0xf, 1);
        gfx_draw_circle(55, 55, 50, 0, 0);

		gfx_draw_line(55, 55, 55+ (50*cos_60[angle_id])/1.5, 55 + (50*sin_60[angle_id])/1.5, COLOR_VGA_MEDIUM_GRAY);
		gfx_draw_line(55, 55, 55+ (50*cos_60[now.minute])/1.1, 55 + (50*sin_60[now.minute])/1.1, COLOR_VGA_MEDIUM_GRAY);
		gfx_draw_line(55, 55, 55+ (50*cos_60[now.second])/1.1, 55 + (50*sin_60[now.second])/1.1, COLOR_VGA_RED);

        gfx_draw_format_text(center_x(5), 112, COLOR_VGA_MEDIUM_GRAY, "%s%d:%s%d", now.hour > 9 ? "" : "0", now.hour, now.minute > 9 ? "" : "0", now.minute);
        gfx_draw_format_text(center_x(6), 124, COLOR_VGA_MEDIUM_GRAY, "%d. %s", now.day, months[now.month]);

        gfx_draw_format_text(0, 0, COLOR_WHITE, "%d", shared);
        sleep(100);

        int ret = gfx_get_event(&event, GFX_EVENT_NONBLOCKING);
        if(ret == -1) continue;

        switch (event.event){
        case GFX_EVENT_EXIT:
            exit();
            break;
        default:
            break;
        }

    }
	return 0;
}