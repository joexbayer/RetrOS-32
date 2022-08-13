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
#include <syscall_helper.h>

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
	init_networking();
	init_dhcpd();

	CLI();
	init_fs();
	
	start_process(0); // SHELL
	start_process(1); // Networking

	add_system_call(SYSCALL_SCRPUT, (syscall_t)&scrput);

	dbgprintf("TEXT: %d\n", _code_end-_code);
	dbgprintf("RODATA: %d\n", _ro_e-_ro_s);
	dbgprintf("DATA: %d\n", _data_e-_data_s);
	dbgprintf("BSS: %d\n", _bss_e-_bss_s);
	dbgprintf("Total: %d (%d sectors)\n", _end-_code, ((_end-_code)/512)+2);
	dbgprintf("Kernel reaching too: 0x%x\n", _end-_code);

	load_page_directory(kernel_page_dir);
    //scrprintf(0, 10, "Kernal page: %x", kernel_page_dir);
	enable_paging();

	dbgprintf("Enabled paging!\n");
	
	//while(1){};
	STI();
	init_timer(1);
	start_tasks();

	while(1){};

}