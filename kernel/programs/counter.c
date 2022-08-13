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

static int counters = 0;
static int value = 0;

void add()
{
    value++;
}


void main()
{
    int id = counters++;
    int num = 0;
	while(1)
    {
		num = (num+1) % 100000000;
		if(num % 100000000 == 0)
		{  
            add();
			screen_put(10, 10+id, 'a');
		}
	};
}