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
	create_window(100, 100);

	struct gfx_char test = {
		.color = 112,
		.data = 'J',
		.x = 10,
		.y = 10
	};

	gfx_draw_syscall(GFX_DRAW_CHAR, &test);

	while(j < 10)
	{	
	  	printf("Counter: %d!\n", j);
		sleep(50);
		j++;
	}

	return 0;
}
