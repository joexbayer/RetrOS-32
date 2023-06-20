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
#include <fs/fs.h>

#include <serial.h>

#include <diskdev.h>

#include <gfx/gfxlib.h>
#include <gfx/theme.h>
#include <gfx/composition.h>
#include <gfx/events.h>

#include <kutils.h>
#include <script.h>

#define SHELL_HEIGHT 275 /* 275 */
#define SHELL_WIDTH 400 /* 400 */
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
	kernel_gfx_draw_rectangle(0, SHELL_POSITION, gfx_get_window_height(), 8, theme->terminal.background);
}

void reset_shell()
{
	shell_clear();
	memset(&shell_buffer, 0, SHELL_MAX_SIZE);
	shell_column = strlen(shell_name)+1;
	shell_buffer_length = 0;
	kernel_gfx_draw_text(0, SHELL_POSITION, shell_name, COLOR_VGA_MISC);
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

void xxd(int argc, char* argv[])
{
	if(argc == 1){
		twritef("usage: xxd <file>\n");
		return;
	}

	inode_t inode = fs_open(argv[1], 0);
	if(inode <= 0){
		twritef("File %s not found.\n", argv[1]);
		return;
	}

	char* buf = kalloc(MAX_FILE_SIZE);
	int ret = fs_read(inode, buf, MAX_FILE_SIZE);
	
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

	inode_t inode = fs_open(argv[1], 0);
	if(inode <= 0){
		twritef("File %s not found.\n", argv[1]);
		return;
	}

	char* buf = kalloc(MAX_FILE_SIZE);
	int ret = fs_read(inode, buf, MAX_FILE_SIZE);
	
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
	char* optarg = NULL;
	char* file = NULL;
    int opt = 0;
	int kpriv = 0;

	while ((opt = getopt(argc, argv, "hkc:", &optarg)) != -1) {
		dbgprintf("%c\n", opt);
        switch (opt) {
            case 'h':
                twritef("run [hn]\n  n - name\n  h - help\nk - launch with kernel permissions\n  example: run -c /bin/clock\n");
                return;
			case 'k':
				kpriv = 1;
				break;
			case 'c':
				file = optarg;
				break;
            case '?':
                twritef("Invalid option\n");
				return;
            case ':':
                twritef("Missing option argument\n");
				return;
            default:
                twritef("Unknown option %c\n", opt);
				return;
        }
	}

	int r = start(optarg);
	if(r >= 0){
		twritef("Kernel thread started\n");
		return;
	}

	int pid = pcb_create_process(optarg, argc, NULL, kpriv ? PCB_FLAG_KERNEL : 0);
	if(pid < 0)
		twritef("%s does not exist\n", file);
	
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
	dbgprintf("%d\n", 0x1337);
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


void cd(int argc, char* argv[])
{
	current_running->current_directory = change_directory(argv[1]);
}
EXPORT_KSYMBOL(cd);

void cat(int argc, char* argv[])
{
	inode_t inode = fs_open(argv[1], 0);
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

void ls()
{
	listdir();
}
EXPORT_KSYMBOL(ls);

void help()
{
	twritef("Help:\n  run - Run a new thread / process.\n  th - Change theme\n  ths - List themes\n");
}
EXPORT_KSYMBOL(help);


char* welcome = "\n\
       _..--=--..._\n\
    .-'            '-.  .-.\n\
   /.'              '.\\/  /\n\
  |=-                -=| (  NETOS\n\
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
		kernel_gfx_draw_rectangle(shell_column*8, SHELL_POSITION, 8, 8, COLOR_VGA_BG);
		gfx_commit();
		shell_buffer[shell_buffer_length] = 0;
		shell_buffer_length--;
		return;
	}

	if(shell_column == SHELL_MAX_SIZE)
		return;

	kernel_gfx_draw_char(shell_column*8, SHELL_POSITION, uc, COLOR_VGA_FG);
	gfx_commit();
	shell_buffer[shell_buffer_length] = uc;
	shell_buffer_length++;
	shell_column++;
}

#include <gfx/api.h>

int c_test = 0;
void shell()
{
	dbgprintf("shell is running!\n");

	memset(term.textbuffer, 0, TERMINAL_BUFFER_SIZE);
	struct gfx_window* window = gfx_new_window(SHELL_WIDTH, SHELL_HEIGHT, GFX_IS_RESIZABLE);
	
	dbgprintf("shell: window 0x%x\n", window);
	kernel_gfx_draw_rectangle(0,0, gfx_get_window_width(), gfx_get_window_height(), COLOR_VGA_BG);

	terminal_attach(&term);

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
		default:
			break;
		}
	}
	
	kernel_exit();
}
EXPORT_KTHREAD(shell);