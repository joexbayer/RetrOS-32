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
    	int test = 0;
		for (int i = 0; i < 40000000; i++)
		{
			test = (test + 1) % 10000;
		}
		j++;

		if(j == 100)
			return;
	}
}
