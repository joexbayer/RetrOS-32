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
		i = (i + 1) % 10000;
		screen_put(0+i, 0, ' a');
	}
}