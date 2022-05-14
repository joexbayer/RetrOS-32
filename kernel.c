#include "util.h"
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

	while(1){};

}