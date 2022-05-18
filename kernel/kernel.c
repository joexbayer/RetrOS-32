#include <util.h>
#include <pci.h>
#include <terminal.h>
#include <keyboard.h>
#include <interrupts.h>
#include <timer.h>
#include <screen.h>

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
	scrwrite(1, 1, "Running... !", VGA_COLOR_WHITE);

	int test_int = 0;
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

	int mem = 0;
	while(1){
		test_int = (test_int+1) % 100000000;
		if(test_int % 100000000 == 0)
		{
			draw_mem_usage(mem++);
		}
	};

}