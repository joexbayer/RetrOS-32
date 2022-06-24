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

#include <process.h>
#include <sync.h>
#include <util.h>
#include <screen.h>

#include <ata.h>
#include <terminal.h>

static int counters = 0;
static int value = 0;
static mutex_t c_lock;

void reset_value()
{
    value = 0;
}

void add()
{
    acquire(&c_lock);
    value++;
    release(&c_lock);
}


void counter()
{
    int id = counters++;

    int num = 0;
	while(1)
    {
		num = (num+1) % 100000000;
		if(num % 100000000 == 0)
		{  
            add();
			scrprintf(10, 10+id, "Counter: %d   ", value);
		}
	};

}

void test()
{
     uint32_t* target = alloc(512);

     twritef("Starting ATA...\r\n");
    read_sectors_ATA_PIO(target, 100, 1);
    
    int i;
    i = 0;
    while(i < 128)
    {
        twritef("%x ", target[i] & 0xFF);
        twritef("%x ", (target[i] >> 8) & 0xFF);
        i++;
    }

    twritef("\r\n");
    twritef("writing 0...\r\n");
    char bwrite[512];
    for(i = 0; i < 512; i++)
    {
        bwrite[i] = 0x1;
    }
    write_sectors_ATA_PIO(100, 2, bwrite);


    twritef("reading...\r\n");
    read_sectors_ATA_PIO(target, 100, 1);
    
    i = 0;
    while(i < 128)
    {
        twritef("%x ", target[i] & 0xFF);
        twritef("%x ", (target[i] >> 8) & 0xFF);
        i++;
    }
}

PROGRAM(counter, &counter)
mutex_init(&c_lock);
ATTACH("reset", &reset_value)
ATTACH("test", &test);
PROGRAM_END