#include "util.h"
#include "pci.h"
#include "terminal.h"
#include "interrupts.h"

void _main(uint32_t debug) {
    /* Initialize terminal interface */
	terminal_initialize();
	init_interrupts();

	if(debug == 0xDEADBEEF)
	{
		terminal_writestring("Hello world\n");
	}
	terminal_write_position(1, 1, "Running... !");

	/* Testing printing ints and hex */
	char test[1000];
	itohex(3735928559, test);
	terminal_writestring(test);
	terminal_writestring("\n");
	itoa(123123, test);
	terminal_writestring(test);
	terminal_writestring("\n");

	/* Testing PCI */
	int dev = pci_find_device(0x8086, 0x100E);
	if(dev){
		terminal_writestring("PCI Device 0x100E Found!\n");
	}

	/* Test interrupt */
	asm volatile ("int $32");
	//asm volatile ("int $31");

	while(1){};

}