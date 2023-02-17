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
#include <rtc.h>

static char* months[] = {"NAN", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Dec"};

int main()
{
    struct time current_time;
    create_window(200, 100);

    while (1)
    {
        get_current_time(&current_time);
        
        //gfx_write_text(2, 2, "Clock:", 0);

        sleep(1000);
    }
    
    printf("Clock started\n");
	return 0;
}
