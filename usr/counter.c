/**
 * @file counter.c
 * @author Joe Bayer (joexbayer)
 * @brief Simple counter program, mainly used for testing processes.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <lib/printf.h>
#include <lib/graphics.h>

int main()
{
	int j = 1;
	gfx_create_window(100, 100);
	printf("Counter started.\n");
	while(j < 10)
	{	
		gfx_draw_rectangle(46, 46, 8, 8, 28);
		gfx_draw_char(46, 46, 48+j, 15);	
		sleep(50);
		j++;
	}

	return 0;
}