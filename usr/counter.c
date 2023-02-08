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

int main()
{
	int j = 1;
	create_window(100, 100);
	while(j < 10)
	{	
	  	printf("Counter: %d!\n", j);
		sleep(50);
		j++;
	}

	return 0;
}
