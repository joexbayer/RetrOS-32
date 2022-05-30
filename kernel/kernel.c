#include <util.h>
#include <pci.h>
#include <terminal.h>
#include <keyboard.h>
#include <interrupts.h>
#include <timer.h>
#include <screen.h>
#include <pcb.h>
#include <memory.h>
#include <shell.h>


/* This functions always needs to be on top? */
void _main(uint32_t debug) {
    /* Initialize terminal interface */
	CLI();
	init_terminal();
	init_shell();
	init_interrupts();
	init_keyboard();
	init_memory();
	init_pcbs();
	init_pci();
	init_timer(1);

	if(debug == 0xDEADBEEF)
	{
		twrite("Hello world\n");
	}
	

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
	//asm volatile ("int $31");

	//draw_mem_usage(10);
	add_pcb(&shell_process, "Shell");
	STI();

	start_tasks();

	while(1){};

}