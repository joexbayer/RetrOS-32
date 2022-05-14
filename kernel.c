#include "util.h"
#include "terminal.h"

void _main() {
    /* Initialize terminal interface */
	terminal_initialize();

	int dev = find_device(0x8086, 0x100E);
	if(dev){
		terminal_writestring("PCI Device 0x100E Found!\n");
	}

    terminal_writestring("Hello world!");
    while(1){};
}
