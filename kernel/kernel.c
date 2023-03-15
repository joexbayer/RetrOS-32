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
#include <ipc.h>
#include <assert.h>
#include <io.h>

#include <gfx/window.h>
#include <gfx/composition.h>
#include <gfx/api.h>

/* This functions always needs to be on top? */
void kernel(uint32_t magic) 
{
	CLI();
	vbe_info = (struct vbe_mode_info_structure*) magic;
	kernel_size = _end-_code;
	
	/* Core functionality */
	init_serial();
	init_memory();
	init_interrupts();
	init_pcbs();
	init_pit(1);
	/* PCI technically not mandetory */
	init_pci(); 

	//vmem_map_driver_region(vbe_info->framebuffer, (vbe_info->width*vbe_info->height*(vbe_info->bpp/8))+1);

	/* Utils? */
	ipc_msg_box_init();

	/* Networking (requires network interface card) */
	init_sk_buffers();
	init_arp();
	init_sockets();
	init_dns();

	/* File System (requires disk device) */
	init_fs();

	/* Graphics */
	vga_set_palette();
	gfx_init();
	init_keyboard();
	mouse_init();
	vesa_init();
	
	/* Kernel threads */
	register_kthread(&shell_main, "Shell");
	register_kthread(&Genesis, "Genesis");
	register_kthread(&networking_main, "Networking");
	register_kthread(&dhcpd, "dhcpd");
	register_kthread(&gfx_compositor_main, "wServer");
	register_kthread(&error_main, "Error");
	register_kthread(&idletask, "Idle");
	register_kthread(&dummytask, "Dummy");

	/*  System calls */
	#pragma GCC diagnostic ignored "-Wcast-function-type"
	add_system_call(SYSCALL_PRTPUT, (syscall_t)&terminal_putchar);
	add_system_call(SYSCALL_EXIT, (syscall_t)&kernel_exit);
	add_system_call(SYSCALL_SLEEP, (syscall_t)&kernel_sleep);
	add_system_call(SYSCALL_GFX_WINDOW, (syscall_t)&gfx_new_window);
	add_system_call(SYSCALL_GFX_GET_TIME,  (syscall_t)&get_current_time);
	add_system_call(SYSCALL_GFX_DRAW, (syscall_t)&gfx_syscall_hook);
	add_system_call(SYSCALL_GFX_SET_TITLE, (syscall_t)&__gfx_set_title);

	add_system_call(SYSCALL_FREE, (syscall_t)&free);
	add_system_call(SYSCALL_MALLOC, (syscall_t)&malloc);

	add_system_call(SYSCALL_OPEN, (syscall_t)&fs_open);
	add_system_call(SYSCALL_READ, (syscall_t)&fs_read);
	add_system_call(SYSCALL_WRITE, (syscall_t)&fs_write);
	#pragma GCC diagnostic pop
	
	/* Print info */
	dbgprintf("[KERNEL] TEXT: %d\n", _code_end-_code);
	dbgprintf("[KERNEL] RODATA: %d\n", _ro_e-_ro_s);
	dbgprintf("[KERNEL] DATA: %d\n", _data_e-_data_s);
	dbgprintf("[KERNEL] BSS: %d\n", _bss_e-_bss_s);
	dbgprintf("[KERNEL] Total: %d (%d sectors)\n", _end-_code, ((_end-_code)/512)+2);
	dbgprintf("[KERNEL] Kernel reaching too: 0x%x\n", _end-_code);

	dbgprintf("[VBE] INFO:\n");
	dbgprintf("[VBE] Height: %d\n", vbe_info->height);
	dbgprintf("[VBE] Width: %d\n", vbe_info->width);
	dbgprintf("[VBE] Pitch: %d\n", vbe_info->pitch);
	dbgprintf("[VBE] Bpp: %d\n", vbe_info->bpp);
	dbgprintf("[VBE] Memory Size: %d (0x%x)\n", vbe_info->width*vbe_info->height*(vbe_info->bpp/8), vbe_info->width*vbe_info->height*(vbe_info->bpp/8));

	/* Start mandetory tasks */
	start("Idle");
	start("wServer");
	//start("Genesis");

	/* Start optional tasks  */
	start("Dummy");
	start("Dummy");
	start("Dummy");
	start("Shell");

	STI();

	/* Kick start first PCB */
	pcb_start();
	
	UNREACHABLE();
}
