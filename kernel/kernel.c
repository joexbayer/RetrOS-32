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
#include <serial.h>

/* This functions always needs to be on top? */
void _main() 
{
	kernel_size = _end-_code;
    /* Initialize terminal interface */
	init_terminal();
	init_memory();
	init_interrupts();
	init_paging();
	CLI();
	init_keyboard();
	init_pcbs();
	ata_ide_init();
	init_serial();

	init_pci();
	init_sk_buffers();
	init_arp();
	init_sockets();
	init_dns();

	/* Programs defined in programs.h */
	init_shell();
	init_counter();
	init_networking();
	init_dhcpd();

	CLI();
	init_fs();
	
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
	start_process(0); // SHELL
	start_process(2); // Networking

	/*twritef("TEXT: %d\n", _code_end-_code);
	twritef("RODATA: %d\n", _ro_e-_ro_s);
	twritef("DATA: %d\n", _data_e-_data_s);
	twritef("BSS: %d\n", _bss_e-_bss_s);
	twriteln("");
	twritef("Total: %d (%d sectors)\n", _end-_code, ((_end-_code)/512)+2);*/

	load_page_directory(kernel_page_dir);
    //scrprintf(0, 10, "Kernal page: %x", kernel_page_dir);
	enable_paging();

	dbgprintf("Enabled paging!\n");
	/*twrite("\n");
	twriteln("~~~ Welcome to NETOS! ~~~");
	twriteln("                  .           o");
    twriteln("      .---.");
    twriteln("=   _/__~0_\\_     .  *            o       '");
    twriteln("= = (_________)             .");
    twriteln("               .                        *");
    twriteln("     *               - ) -       *");
    twriteln("            .               .");
	twriteln("");*/
	
	
	//while(1){};
	STI();
	init_timer(1);
	start_tasks();

	while(1){};

}