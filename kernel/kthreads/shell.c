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

#include <net/socket.h>
#include <net/tcp.h>

#include <kutils.h>
#include <script.h>
#include <vbe.h>

#define SHELL_HEIGHT 225 /* 275 */
#define SHELL_WIDTH 375 /* 300 */
#define SHELL_POSITION shell_height-8
#define SHELL_MAX_SIZE SHELL_WIDTH/2

void __kthread_entry shell(int argc, char* argv[]);

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

static struct terminal* term = NULL; 
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

void ifconfig()
{
	net_list_ifaces();
}
EXPORT_KSYMBOL(ifconfig);

/* Shell commands */
void ps()
{
	int ret;
	int usage;
	term->ops->writef(term, "  PID  USAGE    TYPE     STATE     NAME\n");
	for (int i = 1; i < MAX_NUM_OF_PCBS; i++){
		struct pcb_info info;
		ret = pcb_get_info(i, &info);
		if(ret < 0) continue;
		usage = (int)(info.usage*100);
		term->ops->writef(term, "   %d    %s%d%      %s  %s  %s\n", info.pid, usage < 10 ? " ": "", usage, info.is_process ? "process" : "kthread", pcb_status[info.state], info.name);
	}
}
EXPORT_KSYMBOL(ps);

/* Function to print the lines and corners */
static void print_branches(int level) {
    for (int i = 0; i < level; i++) {
        if (i == level - 1) {
            term->ops->writef(term, ":---");
        } else {
            term->ops->writef(term, ":   ");
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
    term->ops->writef(term, ">%s (%d)\n", pcb->name, pcb->pid);

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
}
EXPORT_KSYMBOL(fat16);


void xxd(int argc, char* argv[])
{
	if(argc == 1){
		term->ops->writef(term, "usage: xxd <file>\n");
		return;
	}

	inode_t inode = fs_open(argv[1], FS_FILE_FLAG_READ);
	if(inode < 0){
		term->ops->writef(term, "File %s not found.\n", argv[1]);
		return;
	}

	char* buf = kalloc(32*1024);
	int ret = fs_read(inode, buf, 32*1024);
	if(ret < 0){
		term->ops->writef(term, "Error reading file\n");
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
		term->ops->writef(term, "usage: sh <file>\n");
		return;
	}

	inode_t inode = fs_open(argv[1], FS_FILE_FLAG_READ);
	if(inode <= 0){
		term->ops->writef(term, "File %s not found.\n", argv[1]);
		return;
	}

	char* buf = kalloc(32*1024);
	int ret = fs_read(inode, buf, 32*1024);
	if(ret < 0){
		fs_close(inode);
		term->ops->writef(term, "Error reading file\n");
		return;
	}
	
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
		term->ops->writef(term, "%s does not exist\n", "edit.o");
}
EXPORT_KSYMBOL(ed);

void exec(int argc, char* argv[])
{
	pid_t pid;
	ubyte_t idx = 1;
	bool_t kthread_as_deamon = false;

	if(argc == 1){
		term->ops->writef(term, "usage: exec [options] <file | kfunc> [args ...]\n");
		return;
	}

	/* check for potential options */
	if(argv[1][0] == '-'){
		switch (argv[1][1]){
		case 'd':{
				kthread_as_deamon = true;	
			}
			break;
		default:
			break;
		}
		idx++;
	}

	pid = start(argv[idx], argc-idx, &argv[idx]);
	if(pid >= 0){
		term->ops->writef(term, "Kernel thread started\n");
		return;
	}

	pid = pcb_create_process(argv[idx], argc-idx, &argv[idx], 0 /* PCB_FLAG_KERNEL */);
	if(pid > 0){
		//pcb_await(pid);
		return;
	}

	void (*ptr)(int argc, char* argv[]) = (void (*)(int argc, char* argv[])) ksyms_resolve_symbol(argv[idx]);
	if(ptr == NULL){
		term->ops->writef(term, "Unknown command\n");
		return;
	}

	pid = pcb_create_kthread(ptr, argv[idx], argc-idx, &argv[idx]);
	if(pid > 0){
		if(kthread_as_deamon) pcb_await(pid);
		return;
	}
	
	term->ops->writef(term, "Unknown command\n");
    return;
}
EXPORT_KSYMBOL(exec);

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
		term->ops->writef(term, "%d) %s\n", i, kernel_gfx_get_theme(i)->name);
	}
}
EXPORT_KSYMBOL(ths);

void dig(int argc, char* argv[])
{
	int ret = gethostname(argv[1]);
	term->ops->writef(term, "%s IN (A) %i\n", argv[1], ret);
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

	term->ops->writef(term, "%s\n", argv[1]);
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
		term->ops->writef(term, "No disk device attached\n");
		return;
	}

	term->ops->writef(term, "fdisk:      \n");
	term->ops->writef(term, "Disk:     %s\n", dev->dev->model);
	term->ops->writef(term, "Size:     %d\n", dev->dev->size*512);
	term->ops->writef(term, "Attached: %d\n", dev->attached);
	term->ops->writef(term, "Read:     %x\n", dev->read);
	term->ops->writef(term, "Write:    %x\n", dev->write);
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

	term->ops->writef(term, "Memory:\n");
	term->ops->writef(term, "  Kernel:    %d%s/%d%s\n", kernel.size, kernel.unit, kernel_total.size, kernel_total.unit);
	term->ops->writef(term, "  Permanent: %d%s/%d%s\n", permanent.size, permanent.unit, permanent_total.size, permanent_total.unit);
	term->ops->writef(term, "  Virtual:   %d%s/%d%s\n", virtual.size, virtual.unit, virtual_total.size, virtual_total.unit);
	term->ops->writef(term, "  Total:     %d%s\n", total.size, total.unit);
}
EXPORT_KSYMBOL(meminfo);

void cat(int argc, char* argv[])
{
	inode_t inode = fs_open(argv[1], FS_FILE_FLAG_READ);
	if(inode < 0){
		term->ops->writef(term, "File %s not found.\n", argv[1]);
		return;
	}

	char buf[512];
	fs_read(inode, buf, 512);
	term->ops->writef(term, "%s\n", buf);
	fs_close(inode);
	return;
}
EXPORT_KSYMBOL(cat);

void res(int argc, char* argv[])
{
	term->ops->writef(term, "Screen resolution: %dx%d\n", vbe_info->width, vbe_info->height);
}
EXPORT_KSYMBOL(res);

void ls(int argc, char* argv[])
{
	struct filesystem* fs = fs_get();
	if(fs == NULL){
		term->ops->writef(term, "No filesystem mounted\n");
		return;
	}

	if(fs->ops->list == NULL){
		term->ops->writef(term, "Filesystem does not support listing\n");
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

void socks(void)
{
	term->ops->writef(term, "Sockets:\n");

	struct sockets socks;
	net_get_sockets(&socks);

	for (int i = 0; i < socks.total_sockets; i++){
		struct sock* sock = socks.sockets[i];
		if(sock == NULL) continue;

		term->ops->writef(term, "%d) %i:%d %s %s %s\n  tx: %d  rx: %d\n\n", i, sock->bound_ip == 1 ? 0 : sock->bound_ip, ntohs(sock->bound_port), socket_type_to_str(sock->type), socket_domain_to_str(sock->domain), sock->tcp ? tcp_state_to_str(sock->tcp->state) : "", sock->tx, sock->rx);
	}
}
EXPORT_KSYMBOL(socks);


void reset(int argc, char* argv[])
{
	kernel_gfx_draw_rectangle(current_running->gfx_window, 0,0, gfx_get_window_width(), gfx_get_window_height(), COLOR_VGA_BG);
	term->ops->reset(term);
	reset_shell();
}
EXPORT_KSYMBOL(reset);

void help()
{
	term->ops->writef(term, "Kthreads:\n");
	kthread_list();
	term->ops->writef(term, "Commands:\n");
	ksyms_list();

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
		term->ops->writef(term, "kernel> %s\n", shell_buffer);
		
		ENTER_CRITICAL();
		if(exec_cmd(shell_buffer) < 0){
			term->ops->writef(term, "Unknown command\n");
		}
		LEAVE_CRITICAL();
		
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
	dbgprintf("shell is running %d!\n", cli_cnt);

	//testfn();
	struct window* window = gfx_new_window(SHELL_WIDTH, SHELL_HEIGHT, GFX_IS_RESIZABLE);
	if(window == NULL){
		warningf("Failed to create window for shell");
		return;
	}
	
	dbgprintf("shell: window 0x%x\n", window);
	kernel_gfx_draw_rectangle(current_running->gfx_window, 0,0, gfx_get_window_width(), gfx_get_window_height(), COLOR_VGA_BG);
	
	term = terminal_create();
	term->ops->attach(term);

	struct mem_info minfo;
    get_mem_info(&minfo);

	struct unit used = calculate_size_unit(minfo.kernel.used+minfo.permanent.used);
	struct unit total = calculate_size_unit(minfo.kernel.total+minfo.permanent.total);

	term->ops->writef(term, "_.--*/ \\*--._\nWelcome ADMIN!\n");
	term->ops->writef(term, "%s\n", welcome);
	term->ops->writef(term, "Memory: %d%s/%d%s\n", used.size, used.unit, total.size, total.unit);
	term->ops->writef(term, "Type 'help' for a list of commands\n");
	terminal_commit();

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
EXPORT_KSYMBOL(shell);