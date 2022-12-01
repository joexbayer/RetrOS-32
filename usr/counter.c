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
#include <printf.h>

void main()
{
	int j = 1;

	while(1)
	{	
	  	printf("Counter: %d!\n", j);
		sleep(1);
		j++;

		if(j == 100)
			return;
	}
}
