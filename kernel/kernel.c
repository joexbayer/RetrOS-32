#include <windowmanager.h>
#include <util.h>
#include <pci.h>
#include <terminal.h>
#include <keyboard.h>
#include <interrupts.h>
#include <timer.h>
#include <screen.h>
#include <pcb.h>
#include <memory.h>
#include <net/skb.h>
#include <net/arp.h>
#include <ata.h>
#include <bitmap.h>
#include <net/socket.h>
#include <net/dns.h>
#include <fs/fs.h>
#include <serial.h>
#include <syscall_helper.h>
#include <syscalls.h>
#include <kthreads.h>
#include <scheduler.h>
#include <vbe.h>
#include <mouse.h>

#include <gfx/window.h>
#include <gfx/composition.h>

/* This functions always needs to be on top? */
void _main(uint32_t magic) 
{

	vbe_info = (struct vbe_mode_info_structure*) magic;

	kernel_size = _end-_code;
	init_serial();
	dbgprintf("VBE INFO:\n");
	dbgprintf("Height: %d\n", vbe_info->height);
	dbgprintf("Width: %d\n", vbe_info->width);
	dbgprintf("Pitch: %d\n", vbe_info->pitch);
	dbgprintf("Bpp: %d\n", vbe_info->bpp);
	dbgprintf("Framebuffer: 0x%x\n", vbe_info->framebuffer);
	dbgprintf("Memory Size: %d (0x%x)\n", vbe_info->width*vbe_info->height*(vbe_info->bpp/8), vbe_info->width*vbe_info->height*(vbe_info->bpp/8));

	init_memory();
	init_interrupts();
	init_paging();
	CLI();
	init_keyboard();
	mouse_init();
	init_pcbs();
	ata_ide_init();
	init_wm();

	init_pci();
	init_sk_buffers();
	init_arp();
	init_sockets();
	init_dns();

	CLI();
	init_fs();
	
	register_kthread(&shell_main, "Shell");
	register_kthread(&networking_main, "Networking");
	register_kthread(&dhcpd, "dhcpd");
	register_kthread(&gfx_compositor_main, "wServer");
	register_kthread(&pcb_info, "PCB Status");

	start("Shell");
	start("wServer");
	start("Networking");
	//start("PCB Status");

	add_system_call(SYSCALL_SCRPUT, (syscall_t)&scrput);
	add_system_call(SYSCALL_PRTPUT, (syscall_t)&terminal_putchar);
	add_system_call(SYSCALL_EXIT, (syscall_t)&exit);
	add_system_call(SYSCALL_SLEEP, (syscall_t)&sleep);

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
	
	vesa_init();

	STI();
	init_timer(1);

	start_tasks();

	while(1){};

}
