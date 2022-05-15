#include "util.h"
#include "pci.h"
#include "terminal.h"

void _main(uint32_t debug) {
    /* Initialize terminal interface */
	terminal_initialize();

	int dev = find_device(0x8086, 0x100E);
	if(dev){
		terminal_writestring("PCI Device 0x100E Found!\n");
	}

	if(debug == 0xDEADBEEF)
	{
		terminal_writestring("Hello world\n");
	}

	terminal_write_position(1, 1, "Running... !");

	char test[1000];
	itohex(3735928559, test);
	terminal_writestring(test);
	itoa(3735928559, test);
	terminal_writestring(test);

	while(1){};

}