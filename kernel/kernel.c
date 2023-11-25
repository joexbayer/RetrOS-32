#include <windowmanager.h>
#include <util.h>
#include <pci.h>
#include <terminal.h>
#include <keyboard.h>
#include <arch/interrupts.h>
#include <timer.h>
#include <pcb.h>
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

#define TEXT_COLOR 15  /* White color for text */
#define LINE_HEIGHT 8  /* Height of each line */

static void kernel_boot_printf(const char* message) {
    static int kernel_msg = 0;
    vesa_printf((uint8_t*)vbe_info->framebuffer, 10, 10 + (kernel_msg++ * LINE_HEIGHT), TEXT_COLOR, message);
}

struct kernel_context {
	struct scheduler* sched_ctx;
	struct windowserver* window_server;
	struct memory_info {
		unsigned int extended_memory_low;
		unsigned int extended_memory_high;
	} *total_memory;
} kernel_context;


/* This functions always needs to be on top? */
void kernel(uint32_t magic) 
{
	asm ("cli");
	kernel_context.total_memory = (struct memory_info*) (0x7e00);
#ifdef USE_MULTIBOOT
  	struct multiboot_info* mb_info = (struct multiboot_info*) magic;
	vbe_info->height = mb_info->framebuffer_height;
	vbe_info->width = mb_info->framebuffer_width;
	vbe_info->bpp = mb_info->framebuffer_bpp;
	vbe_info->pitch = mb_info->framebuffer_width;
	vbe_info->framebuffer = mb_info->framebuffer_addr;

	kernel_context.total_memory->extended_memory_low = 8*1024;
	kernel_context.total_memory->extended_memory_high = 0;
#else
	vbe_info = (struct vbe_mode_info_structure*) magic;
	kernel_context.total_memory = (struct memory_info*) (0x7e00);
#endif
	ENTER_CRITICAL();
    init_serial();

	dbgprintf("Memory: 0x%x\n", kernel_context.total_memory->extended_memory_low);
	dbgprintf("Memory: 0x%x\n", kernel_context.total_memory->extended_memory_high);

	//memset(vbe_info->framebuffer, 0x1, VBE_SIZE());

	kernel_boot_printf("Booting OS...");

	dbgprintf("[KERNEL] Kernel starting...\n");

	rgb_init_color_table();

	/* Clear memory and BSS */
	//memset((char*)_bss_s, 0, (unsigned int) _bss_size);
    //memset((char*)0x100000, 0, 0x800000-0x100000);
	//*((uint32_t*)0x0) = 0xBAADF00D;
	dbgprintf("[KERNEL] BSS: 0x%x\n", _bss_s);

	kernel_size = _end-_code;

	memory_map_init(kernel_context.total_memory->extended_memory_low * 1024, kernel_context.total_memory->extended_memory_high * 64 * 1024);
	init_memory();
	kernel_boot_printf("Memory initialized.");

	dbgprintf("[VBE] INFO:\n");
	dbgprintf("[VBE] Height: %d\n", vbe_info->height);
	dbgprintf("[VBE] Width: %d\n", vbe_info->width);
	dbgprintf("[VBE] Pitch: %d\n", vbe_info->pitch);
	dbgprintf("[VBE] Bpp: %d\n", vbe_info->bpp);
	dbgprintf("[VBE] Memory Size: %d (0x%x)\n", vbe_info->width*vbe_info->height*(vbe_info->bpp/8), vbe_info->width*vbe_info->height*(vbe_info->bpp/8));
	//vmem_map_driver_region((uint8_t*)vbe_info->framebuffer, (vbe_info->width*vbe_info->height*(vbe_info->bpp/8))+1);
	
	init_kctors();
	kernel_boot_printf("Kernel constructors initialized.");

	//vga_set_palette();

	init_interrupts();
	kernel_boot_printf("Interrupts initialized.");
	init_keyboard();
	mouse_init();
	kernel_boot_printf("Peripherals initialized.");
	init_pcbs();
	init_pci();
	kernel_boot_printf("PCI initialized.");
	init_worker();

	/* initilize the default scheduler */
	PANIC_ON_ERR(sched_init_default(get_scheduler(), 0));

	kernel_boot_printf("Scheduler initialized.");

	kernel_boot_printf("Hardware initialized.");

	init_arp();
	init_sockets();
	init_dns();

	kernel_boot_printf("Networking initialized.");

	mbr_partition_load();

	/* check for disk */
	if(!disk_attached()){
		virtual_disk_attach();
	}

	kernel_boot_printf("Filesystem initialized.");

	//ext_create_file_system();

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

#pragma GCC diagnostic pop
	

	kernel_boot_printf("Systemcalls initialized.");

	dbgprintf("[KERNEL] TEXT: %d\n", _code_end-_code);
	dbgprintf("[KERNEL] RODATA: %d\n", _ro_e-_ro_s);
	dbgprintf("[KERNEL] DATA: %d\n", _data_e-_data_s);
	dbgprintf("[KERNEL] BSS: %d (0x%x)\n", _bss_e-_bss_s, _bss_s);
	dbgprintf("[KERNEL] Total: %d (%d sectors)\n", _end-_code, ((_end-_code)/512)+2);
	dbgprintf("[KERNEL] Kernel reaching too: 0x%x\n", _end-_code);

	load_page_directory(kernel_page_dir);
	init_gdt();
	init_tss();
	kernel_boot_printf("GDT & TSS initialized.");
	enable_paging();

	kernel_boot_printf("Virtual memory initialized.");

	dbgprintf("[KERNEL] Enabled paging!\n");
	
	vesa_init();

	kernel_boot_printf("Graphics initialized.");

	start("idled", 0, NULL);
	start("wind", 0, NULL);
	start("workd", 0, NULL);

	kernel_boot_printf("Deamons initialized.");
	
	//pcb_create_process("/bin/clock", 0, NULL);
	
	kernel_boot_printf("Timer initialized.");


	init_pit(1);
	kernel_boot_printf("Starting OS...");

	dbgprintf("[KERNEL] %d\n", cli_cnt);
	LEAVE_CRITICAL();
	asm ("sti");

	while (1)
	{
		/* code */
	}
	
	
	PANIC_ON_ERR(0);
	
}

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

#define HEXDUMP_COLS 8
void hexdump(const void *data, int size)
{
    const unsigned char *p = (const unsigned char *)data;
    int i, j;

    for (i = 0; i < size; i += HEXDUMP_COLS) {
        twritef("%p: ", i);

        for (j = 0; j < HEXDUMP_COLS; j++) {
            if (i + j < size)
                twritef("%s%x ", p[i + j] < 16 ? "0" : "", p[i + j]);
            else
                twritef("   ");
            if (j % 8 == 7)
                twritef(" ");
        }
        twritef(" ");

        for (j = 0; j < HEXDUMP_COLS; j++) {
            if (i + j < size)
                twritef("%c", (p[i + j] >= 32 && p[i + j] <= 126) ? p[i + j] : '.');
            else
                twritef(" ");
        }
        twritef("\n");
    }
}