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
#include <net/socket.h>
#include <net/dns.h>

/* This functions always needs to be on top? */
void _main(uint32_t debug) 
{
    /* Initialize terminal interface */
	CLI();
	init_memory();
	init_terminal();
	init_interrupts();
	init_keyboard();
	init_pcbs();
	ata_ide_init();

	init_pci();
	init_sk_buffers();
	init_arp();
	init_sockets();

	/* Programs defined in programs.h */
	init_shell();
	init_counter();
	init_networking();
	init_dhcpd();
	init_dns();

	init_timer(1);
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

	char buf[512];
	for (size_t i = 0; i < 255; i++)
		buf[i] = i;
	ata_write(buf, 1);


	char buf2[512];
	ata_read(buf2, 1);
	for (size_t i = 0; i < 255; i++)
	{
		twritef("%d", buf2[i]);
	}
	

	
	bitmap_t b_test = create_bitmap(512);

	/* Test interrupt */
	//asm volatile ("int $43");
	// asm volatile ("int $31");
	start_process(0); // SHELL
	start_process(2); // Networking
	STI();

	start_tasks();

	while(1){};

}