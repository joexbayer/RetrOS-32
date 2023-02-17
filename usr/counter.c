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
#include <rtc.h>

int main()
{

	int j = 1;
	create_window(100, 100);

	struct gfx_char test = {
		.color = 15,
		.data = '0',
		.x = 2,
		.y = 2
	};

	struct time t;
	get_current_time(&t);

	printf("%d:%d:%d\n", t.hour, t.minute,t.second);


	while(j < 10)
	{	
	  	printf("Counter: %d!\n", j);
		test.data = 48+j;
		test.x += 8;
		gfx_draw_syscall(GFX_DRAW_CHAR, &test);
		sleep(50);
		j++;
	}

	return 0;
}
