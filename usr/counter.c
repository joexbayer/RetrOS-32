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
#include <syscall.h>
void main()
{
	int i = 1;

	while(1)
	{	
		print_put('a');
		int test = 0;
		for (int i = 0; i < 40000000; i++)
		{
			test = (test + 1) % 10000;
		}
		i++;
		
		
	}
}
