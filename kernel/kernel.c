/**
 * @file kernel.c
 * @author Joe Bayer (joexbayer)
 * @brief The kernel entry point.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <kernel.h>

#include <windowmanager.h>
#include <libc.h>
#include <pci.h>
#include <smp.h>
#include <pcb.h>
#include <terminal.h>
#include <keyboard.h>
#include <arch/interrupts.h>
#include <timer.h>
#include <memory.h>
#include <net/skb.h>
#include <net/arp.h>
#include <ata.h>
#include <bitmap.h>
#include <net/socket.h>
#include <net/dns.h>
#include <fs/ext.h>
#include <serial.h>
#include <syscall_helper.h>
#include <syscalls.h>
#include <kthreads.h>
#include <scheduler.h>
#include <vbe.h>
#include <mouse.h>
#include <ipc.h>
#include <assert.h>
#include <arch/io.h>
#include <work.h>
#include <arch/gdt.h>
#include <kutils.h>
#include <errors.h>
#include <mbr.h>
#include <diskdev.h>

#include <arch/tss.h>

#include <virtualdisk.h>

#include <gfx/window.h>
#include <gfx/windowserver.h>
#include <gfx/composition.h>
#include <gfx/api.h>
#include <net/api.h>

#include <colors.h>
#include <fs/fs.h>
#include <multiboot.h>
#include <screen.h>
#include <conf.h>

#define TEXT_COLOR 15  /* White color for text */
#define LINE_HEIGHT 8  /* Height of each line */

struct kernel_context __kernel_context = {
	.services.scheduler = NULL,
	.services.networking = NULL,
	.services.user_manager = NULL,
	.graphics.window_server = NULL,
	.graphics.ctx = NULL,
	.boot_info = NULL,
	.graphic_mode = KERNEL_FLAG_GRAPHICS,
	//.graphic_mode = KERNEL_FLAG_TEXTMODE,
};
struct kernel_context* $kernel = &__kernel_context;
struct kernel_services* $services = &__kernel_context.services;

static void kernel_boot_printf(char* message) {
    static int kernel_msg = 0;
	if(__kernel_context.graphic_mode != KERNEL_FLAG_TEXTMODE){
		vesa_printf((uint8_t*)vbe_info->framebuffer, 10, 10 + (kernel_msg++ * LINE_HEIGHT), TEXT_COLOR, message);
	} else {
		scrwrite(0, kernel_msg++, message, VGA_COLOR_WHITE);
	}
}

/**
 * @brief The kernel entry point.
 * Responsible for initializing the kernel structs,
 * and starting the kernel threads.
 * Magic number depends on the bootloader.
 * @param magic The magic number passed by the bootloader.
 */
void kernel(uint32_t magic) 
{
	ENTER_CRITICAL();

#ifdef GRUB_MULTIBOOT

	/* Update vbe struct based on multiboot info. */
  	struct multiboot_info* mb_info = (struct multiboot_info*) magic;
	vbe_info->height = mb_info->framebuffer_height;
	vbe_info->width = mb_info->framebuffer_width;
	vbe_info->bpp = mb_info->framebuffer_bpp;
	vbe_info->pitch = mb_info->framebuffer_width;
	vbe_info->framebuffer = mb_info->framebuffer_addr;

	__kernel_context.boot_info->extended_memory_low = 8*1024;
	__kernel_context.boot_info->extended_memory_high = 0;
#else

	/* Point VBE to magic input and update total memory. */
	__kernel_context.boot_info = (struct boot_info*) (0x7e00);
	if(__kernel_context.boot_info->textmode == 1){
		__kernel_context.graphic_mode = KERNEL_FLAG_TEXTMODE;
	}

	vbe_info = (struct vbe_mode_info_structure*) magic;
#endif

	/* Calculate kernel size */
	__deprecated kernel_size = _end-_code;

#ifdef KDEBUG_SERIAL
	/* Serial is used for debuging purposes. */
    init_serial();
#endif
	dbgprintf("INF: %s - %s\n", KERNEL_NAME, KERNEL_VERSION);

	kernel_boot_printf("Booting OS...");
	
	smp_parse();

	/* Initilize memory map and then kernel and virtual memory */
	memory_map_init(__kernel_context.boot_info->extended_memory_low * 1024, __kernel_context.boot_info->extended_memory_high * 64 * 1024);
	init_memory();
	kernel_boot_printf("Memory initialized.");
	
	/* Initilize the kernel constructors */
	init_kctors();
	init_interrupts();
	init_pcbs();
	init_pci();
	init_worker();
	kernel_boot_printf("Kernel constructors initialized.");

	/* Graphics */
	if(__kernel_context.graphic_mode != KERNEL_FLAG_TEXTMODE){

		__kernel_context.graphics.ctx = gfx_new_ctx();
		gfx_init_framebuffer(__kernel_context.graphics.ctx, vbe_info);
		kernel_boot_printf("Graphics initialized.");
	} else {
		scr_clear();
		scrwrite(0, 0, "Welcome to RetrOS32-32 textmode.", VGA_COLOR_WHITE);
	}
	
	/* Initilize the keyboard and mouse */
	init_keyboard();
	if(__kernel_context.graphic_mode != KERNEL_FLAG_TEXTMODE){
		mouse_init();
	}
	kernel_boot_printf("Peripherals initialized.");

	/* initilize the default scheduler */
	PANIC_ON_ERR(sched_init_default(get_scheduler(), 0));
	kernel_boot_printf("Scheduler initialized.");

	/* initilize net structs */
	net_init_arp();
	net_init_sockets();
	net_init_dns();
	net_init_loopback();
	kernel_boot_printf("Networking initialized.");

	/* initilize file systems and disk */
	if(!disk_attached()){
		
	} else {
		mbr_partition_load();
	}
	kernel_boot_printf("Filesystem initialized.");

	register_kthread(&Genesis, "Genesis");
	register_kthread(&networking_main, "netd");
	register_kthread(&dhcpd, "dhcpd");
	register_kthread(&gfx_compositor_main, "wind");
	register_kthread(&idletask, "idled");
	register_kthread(&worker_thread, "workd");
	register_kthread(&tcpd, "tcpd");
	kernel_boot_printf("Kernel Threads initialized.");

#pragma GCC diagnostic ignored "-Wcast-function-type"
	add_system_call(SYSCALL_PRTPUT, (syscall_t)&terminal_putchar);
	
	add_system_call(SYSCALL_EXIT, (syscall_t)&kernel_exit);
	add_system_call(SYSCALL_SLEEP, (syscall_t)&kernel_sleep);
	add_system_call(SYSCALL_YIELD, (syscall_t)&kernel_yield);

	add_system_call(SYSCALL_GFX_WINDOW, (syscall_t)&gfx_new_window);
	add_system_call(SYSCALL_GFX_GET_TIME,  (syscall_t)&get_current_time);
	add_system_call(SYSCALL_GFX_DRAW, (syscall_t)&gfx_syscall_hook);
	add_system_call(SYSCALL_GFX_SET_TITLE, (syscall_t)&kernel_gfx_set_title);
	add_system_call(SYSCALL_GFX_SET_HEADER, (syscall_t)&kernel_gfx_set_header);


	add_system_call(SYSCALL_FREE, (syscall_t)&free);
	add_system_call(SYSCALL_MALLOC, (syscall_t)&malloc);

	add_system_call(SYSCALL_OPEN, (syscall_t)&fs_open);
	add_system_call(SYSCALL_READ, (syscall_t)&fs_read);
	add_system_call(SYSCALL_WRITE, (syscall_t)&fs_write);
	add_system_call(SYSCALL_CLOSE, (syscall_t)&fs_close);
	kernel_boot_printf("Systemcalls initialized.");
#pragma GCC diagnostic pop

	load_page_directory(kernel_page_dir);
	init_gdt();
	init_tss();
	enable_paging();
	kernel_boot_printf("Virtual memory initialized.");

	dbgprintf("[KERNEL] Enabled paging!\n");
	
	/* Initilize the kernel symbols from symbols.map */
	ksyms_init();

	start("idled", 0, NULL);
	if(__kernel_context.graphic_mode != KERNEL_FLAG_TEXTMODE){
		start("wind", 0, NULL);
	} else {
		start("textshell", 0, NULL);	
	}
	start("workd", 0, NULL);
	start("netd", 0, NULL);
	kernel_boot_printf("Deamons initialized.");

	config_load("sysutil/default.cfg");

	$services->user_manager = usermanager_create();
	$services->user_manager->ops->load($services->user_manager);

	init_pit(1000);
	kernel_boot_printf("Timer initialized.");

	dbgprintf("Critical counter: %d\n", __cli_cnt);
	
	kernel_boot_printf("Starting OS...");
	LEAVE_CRITICAL();
	
	while (1);	
}

/**
 * @brief 	Initializes the kernel constructors 
 * @note 	Kernel constructors are functions that are called before the kernel starts
 */
void init_kctors()
{
    int symbols = ((int)_kctor_table_size)/4;
    dbgprintf("%d total kernel constructors\n", symbols);

    unsigned int* __address = (unsigned int*)_start_kctor_table;
    for (int i = 0; i < symbols; i++){
        void (*__func)() = (void (*)()) *__address;
        __func();
        __address++;
    }
}

struct kernel_context* kernel_get_context()
{
	return &__kernel_context;
}

#define HEXDUMP_COLS 8
void hexdump(const void *data, int size)
{
    const unsigned char *p = (const unsigned char *)data;
    int i, j;

    for (i = 0; i < size; i += HEXDUMP_COLS) {
        twritef("%p: ", i);

        for (j = 0; j < HEXDUMP_COLS; j++) {
            if (i + j < size){
    	            twritef("%s%x ", p[i + j] < 16 ? "0" : "", p[i + j]);
			} else {
                twritef("   ");
			}
            if (j % 8 == 7)
                twritef(" ");
        }
        twritef(" ");

        for (j = 0; j < HEXDUMP_COLS; j++) {
            if (i + j < size){
                twritef("%c", (p[i + j] >= 32 && p[i + j] <= 126) ? p[i + j] : '.');
			} else {
                twritef(" ");
			}
        }
        twritef("\n");
    }
}
