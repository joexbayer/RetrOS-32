#include <util.h>
#include <pci.h>
#include <terminal.h>
#include <keyboard.h>
#include <interrupts.h>
#include <timer.h>
#include <screen.h>
#include <pcb.h>

void _main(uint32_t debug) {
    /* Initialize terminal interface */
	init_terminal();
	init_shell();
	init_interrupts();
	init_keyboard();
	init_timer(1);

	if(debug == 0xDEADBEEF)
	{
		twrite("Hello world\n");
	}
	char* hello = "Hello!";
	scrprintf(1, 1, "Format Text Test:\nStrings: %s\nInt: %d\nHex: 0x%x", hello, 10, 2412345);

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
	asm volatile ("int $33");
	//asm volatile ("int $31");

	draw_mem_usage(10);

	init_pcbs();

	while(1){};

}