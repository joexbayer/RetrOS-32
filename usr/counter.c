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
	struct gfx_char number = {
		.color = 15,
		.data = '0',
		.x = 46,
		.y = 46
	};

	struct gfx_rectangle rect = {
		.color = 28,
		.x = 46,
		.y = 46,
		.width = 8,
		.height = 8
	};

	int j = 1;
	create_window(100, 100);
	while(j < 10)
	{	
	  	printf("Counter started.\n");
		number.data = 48+j;
		gfx_draw_syscall(GFX_DRAW_RECTANGLE_OPT, &rect);
		gfx_draw_syscall(GFX_DRAW_CHAR_OPT, &number);
		sleep(50);
		j++;
	}

	return 0;
}
