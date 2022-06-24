#include <util.h>
#include <pci.h>
#include <terminal.h>
#include <keyboard.h>
#include <interrupts.h>
#include <timer.h>
#include <screen.h>
#include <pcb.h>
#include <memory.h>
#include <process.h>
#include <programs.h>
#include <net/skb.h>
#include <net/arp.h>
#include <ata.h>
#include <bitmap.h>

/* This functions always needs to be on top? */
void _main(uint32_t debug) 
{
    /* Initialize terminal interface */
	CLI();
	init_terminal();
	init_interrupts();
	init_timer(1);
	init_keyboard();
	init_memory();
	init_pcbs();

	init_pci();
	init_sk_buffers();
	init_arp();

	/* Programs defined in programs.h */
	init_shell();
	init_counter();
	init_networking();

	/* Testing printing ints and hex */
	char test[1000];
	itohex(3735928559, test);
	twrite(test);
	twrite("\n");

	/* Testing PCI */
	int dev = pci_find_device(0x8086, 0x100E);
	if(dev){
		twrite("PCI Device 0x100E Found!\n");
	}

	/* Test interrupt */
	//asm volatile ("int $43");
	asm volatile ("int $31");

	bitmap_t b_test = create_bitmap(512);

	char* target = alloc(512);

    read_sectors_ATA_PIO(target, 100, 1);
	int i;
    i = 0;
    while(i < 512)
    {
        twritef("%x", target[i]);
        i++;
    }
	twritef("\n");

	char bwrite[512];
    for(i = 0; i < 512; i++)
    {
        bwrite[i] = get_free_bitmap(b_test, 512);
    }
    write_sectors_ATA_PIO(bwrite, 100, 1);

	twritef("reading...\r\n");
    read_sectors_ATA_PIO(target, 100, 1);
    
    i = 0;
    while(i < 512)
    {
        twritef("%x", target[i]);
        i++;
    }
	twritef("\n");


	start_process(0); // SHELL
	STI();

	start_tasks();

	while(1){};

}