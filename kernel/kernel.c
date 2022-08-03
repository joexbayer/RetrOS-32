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
#include <fs/fs.h>

/* This functions always needs to be on top? */
void _main() 
{
    /* Initialize terminal interface */
	init_memory();
	init_terminal();
	init_paging();
	init_interrupts();
	CLI();
	//init_keyboard();
	//init_pcbs();
	//ata_ide_init();

	//init_pci();
	//init_sk_buffers();
	//init_arp();
	//init_sockets();
	//init_dns();

	/* Programs defined in programs.h */
	//init_shell();
	//init_counter();
	//init_networking();
	//init_dhcpd();

	//init_fs();


	init_timer(1);
	/* Testing printing ints and hex */
	//char test[1000];
	//itohex(3735928559, test);
	//twrite(test);
	//write("\n");

	/* Testing PCI */
	//int dev = pci_find_device(0x8086, 0x100E);
	//if(dev){
	//	twrite("PCI Device 0x100E Found!\n");
	//}
	
	//bitmap_t b_test = create_bitmap(512);

	/* Test interrupt */
	//start_process(0); // SHELL
	//start_process(2); // Networking

	loadPageDirectory(kernel_page_dir);
    scrprintf(0, 10, "Kernal page: %x", kernel_page_dir);
	enablePaging();

	STI();
	while (1)
	{
		/* code */
	}
	
	
	start_tasks();

	while(1){};

}