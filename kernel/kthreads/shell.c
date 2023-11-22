/**
 * @file shell.c
 * @author Joe Bayer (joexbayer)
 * @brief Simple program handling input from user, mainly used to handles process management.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <editor.h>
#include <pci.h>
#include <keyboard.h>
#include <terminal.h>
#include <scheduler.h>
#include <pcb.h>
#include <rtc.h>
#include <kthreads.h>
#include <ksyms.h>
#include <arch/io.h>

#include <windowmanager.h>
#include <net/dns.h>
#include <net/icmp.h>
#include <fs/ext.h>

#include <serial.h>

#include <diskdev.h>

#include <gfx/gfxlib.h>
#include <gfx/theme.h>
#include <gfx/composition.h>
#include <gfx/events.h>

#include <fs/fs.h>
#include <fs/fat16.h>


#include <kutils.h>
#include <script.h>
#include <vbe.h>

#define SHELL_HEIGHT 225 /* 275 */
#define SHELL_WIDTH 350 /* 300 */
#define SHELL_POSITION shell_height-8
#define SHELL_MAX_SIZE SHELL_WIDTH/2


/* TODO: Move this into a shell struct */
static uint16_t shell_width = SHELL_WIDTH;
static uint16_t shell_height = SHELL_HEIGHT;
static uint8_t shell_column = 0;
static char previous_shell_buffer[SHELL_MAX_SIZE];
static char shell_buffer[SHELL_MAX_SIZE];
static uint8_t shell_buffer_length = 0;

static const char newline = '\n';
static const char backspace = '\b';

static char* shell_name = "Kernel >";

static struct terminal term  = {
	.head = 0,
	.tail = 0,
	.lines = 0
};

/*
 *	IMPLEMENTATIONS
 */
void shell_clear()
{
	struct gfx_theme* theme = kernel_gfx_current_theme();
	kernel_gfx_draw_rectangle(current_running->gfx_window, 0, SHELL_POSITION, gfx_get_window_height(), 8, theme->terminal.background);
}

void reset_shell()
{
	shell_clear();
	memset(&shell_buffer, 0, SHELL_MAX_SIZE);
	shell_column = strlen(shell_name)+1;
	shell_buffer_length = 0;
	kernel_gfx_draw_text(current_running->gfx_window, 0, SHELL_POSITION, shell_name, COLOR_VGA_MISC);
	shell_column += 1;
}

/* Shell commands */
void ps()
{
	int ret;
	int line = 0;
	int usage;
	twritef("  PID  USAGE    TYPE     STATE     NAME\n");
	for (int i = 1; i < MAX_NUM_OF_PCBS; i++){
		struct pcb_info info;
		ret = pcb_get_info(i, &info);
		if(ret < 0) continue;
		usage = (int)(info.usage*100);
		twritef("   %d    %s%d%      %s  %s  %s\n", info.pid, usage < 10 ? " ": "", usage, info.is_process ? "process" : "kthread", pcb_status[info.state], info.name);
	}
}
EXPORT_KSYMBOL(ps);

/* Function to print the lines and corners */
static void print_branches(int level) {
    for (int i = 0; i < level; i++) {
        if (i == level - 1) {
            twritef(":---");
        } else {
            twritef(":   ");
        }
    }
}

/* Recursive function to print PCB tree */
static void print_pcb_tree(struct pcb *pcb, int level) {
    if (pcb == NULL || pcb->pid == -1) {
        return;
    }

    /* Print branches and nodes */
    print_branches(level);
    twritef(">%s\n", pcb->name);

    /* Recursively print child PCBs */
    for (int i = 1; i < MAX_NUM_OF_PCBS; i++) {
        struct pcb *child_pcb = pcb_get_by_pid(i);
        if (child_pcb && child_pcb->parent == pcb) {
            print_pcb_tree(child_pcb, level + 1);
        }
    }
}

/* Function to visualize the PCBs as a tree */
void tree() {
    /* Hierarchical visualization of the PCBs based on their parent */
    for (int i = 1; i < MAX_NUM_OF_PCBS; i++) {
        struct pcb *pcb = pcb_get_by_pid(i);
        if (pcb && pcb->parent == NULL) { /* Start with root PCBs */
            print_pcb_tree(pcb, 0);
        }
    }
}
EXPORT_KSYMBOL(tree);

void fat16(){
	fat16_format("VOLUME1", 1);
	fat16_init();

	/* TODO USE FS */
	fat16_create_directory("BIN     ");
	fat16_change_directory("BIN     ");

	/* load editor */
	int fd = fs_open("edit.o", FS_FILE_FLAG_CREATE | FS_FILE_FLAG_WRITE);
	if(fd < 0){
		twritef("Failed to create file\n");
		return;
	}

	int ret = fs_write(fd, (char*)apps_editor_edit_o, apps_editor_edit_o_len);
	if(ret < 0){
		twritef("Failed to write file\n");
		return;
	}

	fs_close(fd);
}
EXPORT_KSYMBOL(fat16);


void xxd(int argc, char* argv[])
{
	if(argc == 1){
		twritef("usage: xxd <file>\n");
		return;
	}

	inode_t inode = fs_open(argv[1], FS_FILE_FLAG_READ);
	if(inode < 0){
		twritef("File %s not found.\n", argv[1]);
		return;
	}

	char* buf = kalloc(32*1024);
	int ret = fs_read(inode, buf, 32*1024);
	if(ret < 0){
		twritef("Error reading file\n");
		return;
	}
	
	hexdump(buf, ret);
	
	fs_close(inode);
	kfree(buf);

	return;
}
EXPORT_KSYMBOL(xxd);

void sh(int argc, char* argv[])
{
	if(argc == 1){
		twritef("usage: sh <file>\n");
		return;
	}

	inode_t inode = fs_open(argv[1], FS_FILE_FLAG_READ);
	if(inode <= 0){
		twritef("File %s not found.\n", argv[1]);
		return;
	}

	char* buf = kalloc(32*1024);
	int ret = fs_read(inode, buf, 32*1024);
	
	script_parse(buf);
	
	fs_close(inode);
	kfree(buf);

	return;
}
EXPORT_KSYMBOL(sh);

void ed()
{
	int pid = pcb_create_process("/bin/edit.o", 0, NULL, PCB_FLAG_KERNEL);
	if(pid < 0)
		twritef("%s does not exist\n", "edit.o");
}
EXPORT_KSYMBOL(ed);

void run(int argc, char* argv[])
{
	int r = start(argv[1], argc-1, &argv[1]);
	if(r >= 0){
		twritef("Kernel thread started\n");
		return;
	}

	int pid = pcb_create_process(argv[1], argc-1, &argv[1], PCB_FLAG_KERNEL);
	if(pid < 0){
		twritef("%s does not exist\n", argv[1]);
	}
	
    return;
}
EXPORT_KSYMBOL(run);

/**
 * @brief cmd: command [opts] [args]
 * 
 * command:
 * 	resolved from ksyms
 * 
 * opts:
 *  -t run as a thread (long)
 *  -w run as a worker (short)
 * 
 * args: 
 * 	arguments passed to command
 * 
 */

void ths()
{
	int total_themes = gfx_total_themes();
	for (int i = 0; i < total_themes; i++){
		twritef("%d) %s\n", i, kernel_gfx_get_theme(i)->name);
	}
}
EXPORT_KSYMBOL(ths);

void dig(int argc, char* argv[])
{
	int ret = gethostname(argv[1]);
	twritef("%s IN (A) %i\n", argv[1], ret);
}
EXPORT_KSYMBOL(dig);

void th(int argc, char* argv[])
{
	int id = atoi(argv[1]);
	kernel_gfx_set_theme(id);
}
EXPORT_KSYMBOL(th);

void kill(int argc, char* argv[])
{
	int id = atoi(argv[1]);
	pcb_kill(id);
}
EXPORT_KSYMBOL(kill);

void echo(int argc, char* argv[])
{	
	if(argc <= 1){
		return;
	}

	twritef("%s\n", argv[1]);
}
EXPORT_KSYMBOL(echo);

void log(int argc, char* argv[])
{
	int pid = atoi(argv[1]);
	logd_attach_by_pid(pid);
}
EXPORT_KSYMBOL(log);

void cd(int argc, char* argv[])
{
	current_running->current_directory = change_directory(argv[1]);
}
EXPORT_KSYMBOL(cd);

void fdisk(int argc, char* argv[])
{
	struct diskdev* dev = disk_device_get();
	if(dev == NULL){
		twritef("No disk device attached\n");
		return;
	}

	twritef("fdisk:      \n");
	twritef("Disk:     %s\n", dev->dev->model);
	twritef("Size:     %d\n", dev->dev->size*512);
	twritef("Attached: %d\n", dev->attached);
	twritef("Read:     %x\n", dev->read);
	twritef("Write:    %x\n", dev->write);
}
EXPORT_KSYMBOL(fdisk);

void meminfo(int argc, char* argv[])
{
	struct mem_info minfo;
	get_mem_info(&minfo);

	/* write mem for all 3 types */
	struct unit kernel = calculate_size_unit(minfo.kernel.used);
	struct unit kernel_total = calculate_size_unit(minfo.kernel.total);

	struct unit permanent = calculate_size_unit(minfo.permanent.used);
	struct unit permanent_total = calculate_size_unit(minfo.permanent.total);

	struct unit virtual = calculate_size_unit(minfo.virtual.used);
	struct unit virtual_total = calculate_size_unit(minfo.virtual.total);

	struct unit total = calculate_size_unit(minfo.kernel.total+minfo.permanent.total+minfo.virtual.total);

	twritef("Memory:\n");
	twritef("  Kernel:    %d%s/%d%s\n", kernel.size, kernel.unit, kernel_total.size, kernel_total.unit);
	twritef("  Permanent: %d%s/%d%s\n", permanent.size, permanent.unit, permanent_total.size, permanent_total.unit);
	twritef("  Virtual:   %d%s/%d%s\n", virtual.size, virtual.unit, virtual_total.size, virtual_total.unit);
	twritef("  Total:     %d%s\n", total.size, total.unit);
}
EXPORT_KSYMBOL(meminfo);

void cat(int argc, char* argv[])
{
	inode_t inode = fs_open(argv[1], FS_FILE_FLAG_READ);
	if(inode < 0){
		twritef("File %s not found.\n", argv[1]);
		return;
	}

	char buf[512];
	fs_read(inode, buf, 512);
	twritef("%s\n", buf);
	fs_close(inode);
	return;
}
EXPORT_KSYMBOL(cat);

void res(int argc, char* argv[])
{
	twritef("Screen resolution: %dx%d\n", vbe_info->width, vbe_info->height);
}
EXPORT_KSYMBOL(res);

void ls(int argc, char* argv[])
{
	struct filesystem* fs = fs_get();
	if(fs == NULL){
		twritef("No filesystem mounted\n");
		return;
	}

	if(fs->ops->list == NULL){
		twritef("Filesystem does not support listing\n");
		return;
	}

	if(argc == 1){
		fs->ops->list(fs, "/", NULL, 0);
		return;
	}
	fs->ops->list(fs, argv[1], NULL, 0);

	//listdir();
}
EXPORT_KSYMBOL(ls);

void reset(int argc, char* argv[])
{
	kernel_gfx_draw_rectangle(current_running->gfx_window, 0,0, gfx_get_window_width(), gfx_get_window_height(), COLOR_VGA_BG);
	term.head = 0;
	term.tail = 0;
	term.lines = 0;
	memset(term.textbuffer, 0, TERMINAL_BUFFER_SIZE);
	reset_shell();
}
EXPORT_KSYMBOL(reset);

void help()
{
	twritef("Help:\n  run - Run a new thread / process.\n  th - Change theme\n  ths - List themes\n");
}
EXPORT_KSYMBOL(help);


char* welcome = "\n\
       _..--=--..._\n\
    .-'            '-.  .-.\n\
   /.'              '.\\/  /\n\
  |=-                -=| (  RetrOS-32\n\
   \\'.              .'/\\  \\\n\
    '-.,_____ _____.-'  '-'\n\
         [_____]=8\n";

/**
 * @brief Puts a character c into the shell line 
 * at correct position. Also detects enter.
 * 
 * @param c character to put to screen.
 */
void shell_put(unsigned char c)
{
	int ret;
	unsigned char uc = c;

	if(uc == ARROW_UP){
		int len = strlen(previous_shell_buffer)+1;
		for (int i = 0; i < len; i++){
			shell_put(previous_shell_buffer[i]);
		}
		return;
	}

	if(uc == newline){
		memcpy(previous_shell_buffer, shell_buffer, strlen(shell_buffer)+1);
		twritef("kernel> %s\n", shell_buffer);
		if(exec_cmd(shell_buffer) < 0){
			twritef("Unknown command\n");
		}
		
		terminal_commit();
		reset_shell();
		
		return;
	}

	if(uc == backspace)
	{
		if(shell_buffer_length < 1)
			return;
		shell_column -= 1;
		kernel_gfx_draw_rectangle(current_running->gfx_window, shell_column*8, SHELL_POSITION, 8, 8, COLOR_VGA_BG);
		gfx_commit();
		shell_buffer[shell_buffer_length] = 0;
		shell_buffer_length--;
		return;
	}

	if(shell_column == SHELL_MAX_SIZE)
		return;

	kernel_gfx_draw_char(current_running->gfx_window, shell_column*8, SHELL_POSITION, uc, COLOR_VGA_FG);
	gfx_commit();
	shell_buffer[shell_buffer_length] = uc;
	shell_buffer_length++;
	shell_column++;
}

#include <gfx/api.h>

#define CLEANUP_FUNCTION __attribute__((cleanup(cleanup_function)))
void cleanup_function(int** ptr)
{
    dbgprintf("Cleaning up...\n");
    kfree(*ptr);
}


void testfn()
{
	dbgprintf("Testfn...\n");
	CLEANUP_FUNCTION int* ptr = kalloc(sizeof(int) * 50);
}

int c_test = 0;
void __kthread_entry shell(int argc, char* argv[])
{
	dbgprintf("shell is running %d!\n", argc);

	testfn();
	struct window* window = gfx_new_window(SHELL_WIDTH, SHELL_HEIGHT, GFX_IS_RESIZABLE);
	if(window == NULL){
		warningf("Failed to create window for shell");
		return;
	}

	terminal_attach(&term);
	
	dbgprintf("shell: window 0x%x\n", window);
	kernel_gfx_draw_rectangle(current_running->gfx_window, 0,0, gfx_get_window_width(), gfx_get_window_height(), COLOR_VGA_BG);


	struct mem_info minfo;
    get_mem_info(&minfo);

	struct unit used = calculate_size_unit(minfo.kernel.used+minfo.permanent.used);
	struct unit total = calculate_size_unit(minfo.kernel.total+minfo.permanent.total);

	twritef("_.--*/ \\*--._\nWelcome ADMIN!\n");
	twritef("%s\n", welcome);
	twritef("Memory: %d%s/%d%s\n", used.size, used.unit, total.size, total.unit);
	help();
	twriteln("");
	terminal_commit(current_running->term);

	kernel_gfx_set_header("/");

	reset_shell();

	dbgprintf("shell: entering event loop\n");
	while(1)
	{
		struct gfx_event event;
		gfx_event_loop(&event, GFX_EVENT_BLOCKING);

		switch (event.event){
		case GFX_EVENT_KEYBOARD:
			switch (event.data){
			case CTRLC:
				reset_shell();
				break;
			default:
				shell_put(event.data);
				c_test++;
				break;
			}
			break;
		case GFX_EVENT_RESOLUTION:
			shell_height = event.data2;
			shell_width = event.data;
			terminal_commit();
			reset_shell();
			break;
		case GFX_EVENT_EXIT:
			kernel_exit();
			return;
		default:
			break;
		}
	}
	
	kernel_exit();
}
EXPORT_KTHREAD(shell);