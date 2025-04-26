/**
 * @file cmds.c
 * @author Joe Bayer (joexbayer)
 * @brief All commands for the kernel
 * @version 0.1
 * @date 2023-12-24
 * 
 * Commands can be executed using exec_cmd(char *cmd)
 * Take in argc and argv, and can be run in parallel.
 * 
 * @copyright Copyright (c) 2023
 */

#include <kutils.h>
#include <kernel.h>
#include <ksyms.h>
#include <terminal.h>
#include <memory.h>
#include <fs/fs.h>
#include <work.h>
#include <conf.h>
#include <gfx/theme.h>
#include <kevents.h>
#include <lib/lz.h>

#define COMMAND(name, func) \
	int name\
		func\
	EXPORT_KSYMBOL(name);

static int __read_file(char* filename, char** buffer){
    struct filesystem* fs = fs_get();
    if(fs == NULL){
        twritef("No filesystem mounted.\n");
        return -1;
    }

    struct file* f = fs->ops->open(fs, filename, FS_FILE_FLAG_READ);
    if(f == NULL){
        twritef("Failed to open file %s\n", filename);
        return -1;
    }

    *buffer = kalloc(f->size);
    int len = fs->ops->read(fs, f, *buffer, f->size);
    if(len < 0){
        twritef("Failed to read file %s\n", filename);
        return -1;
    }

    fs->ops->close(fs, f);
    return len;
}

static int __write_file(char* filename, char* buffer, int size){
    struct filesystem* fs = fs_get();
    if(fs == NULL){
        twritef("No filesystem mounted.\n");
        return -1;
    }

    struct file* f = fs->ops->open(fs, filename, FS_FILE_FLAG_CREATE | FS_FILE_FLAG_WRITE);
    if(f == NULL){
        twritef("Failed to open file %s\n", filename);
        return -1;
    }

    int len = fs->ops->write(fs, f, buffer, size);
    if(len < 0){
        twritef("Failed to write file %s\n", filename);
        return -1;
    }

    fs->ops->close(fs, f);
    return len;
}


/**
 * @brief Create a file or directory
 * Specified by second argument "file" or "dir"
 */
static int new(int argc, char* argv[])
{
    if(argc < 3) {
        twritef("Usage: new [file, dir] <name>\n");
        return 1;
    }

    if(strcmp(argv[1], "file") == 0) {

        int fd = fs_open(argv[2], FS_FILE_FLAG_CREATE);
        if(fd < 0) {
            twritef("Failed to create file %s\n", argv[2]);
            return 1;
        }

    } else if(strcmp(argv[1], "dir") == 0) {

        twritef("Currently unsupported.\n");
    
    } else {
        twritef("Usage: new <file, dir> <name>\n");
        return 1;
    }

    return 0;
}
EXPORT_KSYMBOL(new);

static int list(int argc, char* argv[])
{
    if(argc < 1) {
        twritef("Usage: list <path?>\n");
        return 1;
    }
    
    struct filesystem* fs = fs_get();
	if(fs == NULL){
		twritef("No filesystem mounted.\n");
		return -1;
	}

	if(fs->ops->list == NULL){
		twritef("Filesystem does not support listing\n");
		return -1;
	}

	if(argc == 1){
		fs->ops->list(fs, "/", NULL, 0);
		return -1;
	}

	fs->ops->list(fs, argv[1], NULL, 0);

    return 0;
}
EXPORT_KSYMBOL(list);

/* prints content of compressed file */
static int lzcat(int argc, char* argv[]){
    if(argc < 2) {
        twritef("Usage: lz <file>\n");
        return 1;
    }
    uint8_t* buf;
    int len = __read_file(argv[1], &buf);

    uint8_t* output;
    uint32_t output_size = lz_decompress(buf, len, &output);
    if(output_size == 0) {
        twritef("Failed to decompress file\n");
        return -1;
    }

    twritef("%s\n", output);

    free(output);
    kfree(buf);
    
    return 0;
}
EXPORT_KSYMBOL(lzcat);

static int lz(int argc, char* argv[]){
    if(argc < 3) {
        twritef("Usage: lz <file> <outfile>\n");
        return 1;
    }
    uint8_t* buf;
    int len = __read_file(argv[1], &buf);

    uint8_t* output;
    uint32_t output_size = lz_compress(buf, len, &output, 0);
    if(output_size == 0) {
        twritef("Failed to compress file\n");
        return -1;
    }

    int ret = __write_file(argv[2], output, output_size);
    if(ret < 0) {
        twritef("Failed to write compressed file\n");
        return -1;
    }

    free(output);
    kfree(buf);
    
    return 0;
}
EXPORT_KSYMBOL(lz);

static int color(int argc, char* argv[]){
    if(argc < 2) {
        twritef("Usage: color <text> <background>\n Use hex values\n");
        return 1;
    }

    color_t text = htoi(argv[1]);
    color_t bg = 255;

    if(argc == 3){
        bg = htoi(argv[2]);
    }

    TERM_CONTEXT({
        term->text_color = text;
        term->org_text_color = text;
        if(bg != 255) {
            term->bg_color = bg;
        }
    });

    return 0;
}
EXPORT_KSYMBOL(color);

static int about(){

    struct memory_map* map = memory_map_get();

    /* neofetch style twritef */
    #define RETROS_ASCII_ART_INFO \
    "                  .----.      Kernel: RetrOS-32 0.0.1 alpha\n" \
    "      .---------. | == |      Build Date: " __DATE__ "\n" \
    "      |.-\"\"\"\"\"-.| |----|      Build Time: " __TIME__ "\n" \
    "      ||       || | == |      Memory: %d bytes\n" \
    "      ||RetrOS || |----|      Display: VGA\n" \
    "      |'-.....-'| |::::|                \n" \
    "      `\"\")---(\"\"` |___.|                \n" \
    "     /:::::::::::\" _  \"                \n" \
    "    /:::=======:::\\`\\`\\             \n" \
    "    `\"\"\"\"\"\"\"\"\"\"\"\"\"`  '-'                \n"

    twritef(RETROS_ASCII_ART_INFO, map->total);

    return 0;
}
EXPORT_KSYMBOL(about);

static int help(int argc, char* argv[])
{
    twritef("Available commands:\n\n");
    twritef("Filesystem:      System:      Etc:\n");
    twritef("  new             ps           help\n");
    twritef("  list            tree         reset\n");
    twritef("  view            kill         about\n");
    twritef("  file            exec         xxd\n");
    twritef("                  fdisk        bg\n");
    twritef("Network:          meminfo      sh\n");
    twritef(" socks            conf         echo\n");
    twritef(" ifconfig         services     cc\n");
    twritef(" dns              admin        color\n");
    twritef(" tcp              clear        \n");
    return 0;
}
EXPORT_KSYMBOL(help);

int joe(int argc, char* argv[])
{
    int a;
    int b;
    int c;

    a = argc;
    b = 2;
    c = a + b;

    return c;
}

static int dump(int argc, char* argv[])
{
    if(argc < 2) {
        twritef("Usage: dump <address>\n");
        return 1;
    }

    void* addr = (void*) htoi(argv[1]);
    hexdump(addr, 256);

    return 0;
}
EXPORT_KSYMBOL(dump);

static int dumpfn(int argc, char* argv[])
{
    if(argc < 2) {
        twritef("Usage: dumpfn <function>\n");
        return 1;
    }

    uintptr_t addr = ksyms_resolve_symbol(argv[1]);
    if(addr == 0) {
        twritef("Symbol not found\n");
        return 1;
    }

    int sz = kfunc_size((void*) addr);
    twritef("Dumping %s (addr: 0x%x, size: %d)\n", argv[1], addr, sz);
    hexdump((void*) addr, sz);

    return 0;
}
EXPORT_KSYMBOL(dumpfn);

static int regs(int argc, char* argv[])
{
    struct registers_dump regs;
    kregisters(&regs);

    twritef("Registers:\n");
    twritef("EAX: %x\n", regs.eax);
    twritef("EBX: %x\n", regs.ebx);
    twritef("ECX: %x\n", regs.ecx);
    twritef("EDX: %x\n", regs.edx);
    twritef("ESI: %x\n", regs.esi);
    twritef("EDI: %x\n", regs.edi);
    twritef("EBP: %x\n", regs.ebp);
    twritef("ESP: %x\n", regs.esp);
    twritef("EIP: %x\n", regs.eip);
    twritef("EFLAGS: %x\n", regs.eflags);
    twritef("CR0: %x\n", regs.cr0);
    twritef("CR2: %x\n", regs.cr2);
    twritef("CR3: %x\n", regs.cr3);
    twritef("CR4: %x\n", regs.cr4);
    twritef("GS: %x\n", regs.gs);
    
    return 0;
}
EXPORT_KSYMBOL(regs);

int syms(int args, char* argv[])
{
    ksyms_list();
    return 0;
}
EXPORT_KSYMBOL(syms);

int disas(int argc, char* argv[])
{
    if(argc < 2) {
        twritef("Usage: disassemble <function>\n");
        return 1;
    }

    uintptr_t addr = ksyms_resolve_symbol(argv[1]);
    if(addr == 0) {
        twritef("Symbol not found\n");
        return 1;
    }

    int sz = kfunc_size((void*) addr);
    twritef("Disassembling %s (addr: 0x%x, size: %d)\n", argv[1], addr, sz);
    disassemble((uint8_t*) addr, sz, addr);

    return 0;
}
EXPORT_KSYMBOL(disas);

int resolve(int argc, char* argv[])
{
    if(argc < 2) {
        twritef("Usage: resolve <symbol>\n");
        return 1;
    }

    uintptr_t addr = ksyms_resolve_symbol(argv[1]);
    if(addr == 0) {
        twritef("Symbol not found\n");
        return 1;
    }

    twritef("Symbol %s resolved to 0x%x\n", argv[1], addr);
    return 0;
}
EXPORT_KSYMBOL(resolve);

static int kevents(int argc, char* argv[])
{
    if(IS_AUTHORIZED(ADMIN_FULL_ACCESS) == 0) {
        twritef("You are not authorized to use this command\n");
        return 1;
    }

    if(argc < 2) {
        twritef("Usage: kevents <list>\n");
        return 1;
    }

    if(strcmp(argv[1], "list") == 0) {
        $services->kevents->ops->list($services->kevents);
    } else {
        twritef("Usage: kevents <list>\n");
        return 1;
    }

    return 0;
}
EXPORT_KSYMBOL(kevents);


static int view(int argc, char* argv[]){
    if(argc < 2) {
        twritef("Usage: view <path>\n");
        return 1;
    }

    struct filesystem* fs = fs_get();
    if(fs == NULL){
        twritef("No filesystem mounted.\n");
        return -1;
    }

    if(fs->ops->read == NULL){
        twritef("Filesystem does not support reading\n");
        return -1;
    }

    struct file* file = fs->ops->open(fs, argv[1], FS_FILE_FLAG_READ);
    if(file == NULL) {
        twritef("Failed to open file %s\n", argv[1]);
        return -1;
    }

    if(file->size == 0) {
        twritef("File %s is empty\n", argv[1]);
        fs->ops->close(fs, file);
        return -1;
    }

    ubyte_t* buf = kalloc(file->size);
    int len = fs->ops->read(fs, file, buf, file->size);
    if(len < 0) {
        twritef("Failed to read file %s\n", argv[1]);
        return -1;
    }

    twritef("%s\n", buf);

    fs->ops->close(fs, file);

    return 0;
}
EXPORT_KSYMBOL(view);

static int find(int argc, char* argv[]){    
    if(argc < 2) {
        twritef("Usage: find <path>\n");
        return 1;
    }

    struct filesystem* fs = fs_get();
    if(fs == NULL){
        twritef("No filesystem mounted.\n");
        return -1;
    }

    if(fs->ops->find == NULL){
        twritef("Filesystem does not support finding\n");
        return -1;
    }

    fs->ops->find(fs, "/", argv[1]);
    return 0;
}
EXPORT_KSYMBOL(find);

static int file(int argc, char* argv[]){
    if(argc < 2) {
        twritef("Usage: file <path>\n");
        return 1;
    }

    struct filesystem* fs = fs_get();
    if(fs == NULL){
        twritef("No filesystem mounted.\n");
        return -1;
    }

    struct file* file = fs->ops->open(fs, argv[1], FS_FILE_FLAG_READ);
    if(file == NULL) {
        twritef("Failed to open file %s\n", argv[1]);
        return -1;
    }

    twritef("File %s:\n", argv[1]);
    twritef("Nlinks: %d\n", file->nlinks);
    twritef("Directory: %d\n", file->directory);
    twritef("Size: %d\n", file->size);

    fs->ops->close(fs, file);
    return 0;
}
EXPORT_KSYMBOL(file);

#include <net/socket.h>
#include <net/net.h>
#include <net/ipv4.h>
#include <net/utils.h>

/**
 * @brief Part of the TCP client
 * 
 * @param argc Should always be 1
 * @param argv Contains socket as int
 * @return int 
 */
static int __kthread_entry __tcp_reader(int argc, char *argv[])
{
    if(argc != 1){
        dbgprintf("Invalid arguments\n");
        return 1;
    }

    struct sock* socket = sock_get(atoi(argv[0]));
    if(socket == NULL) {
        dbgprintf("Invalid socket\n");
        return 1;
    }

    dbgprintf("TCP reader started\n");

    int ret;
    char buffer[1024];
    while(1){
        ret = kernel_recv(socket, buffer, 1024, 0);
        if(ret < 0){
            twritef("Unable to recv TCP packet\n");
            break;
        }

        buffer[ret] = 0;
        twritef("%s", buffer);

        $process->current->term->ops->commit($process->current->term);
    }

    return 0;
}

static int tcp(int argc, char *argv[])
{
    if(argc < 3) {
        twritef("Usage: tcp <ip,domain> <port>\n");
        return 1;
    }

    struct sock* socket = kernel_socket_create(AF_INET, SOCK_STREAM, 0);
    if(socket == NULL) {
        twritef("Failed to create socket\n");
        return 1;
    }

    int ip = net_is_ipv4(argv[1]) ? ip_to_int(argv[1]) : htonl(gethostname(argv[1]));
    if(ip == -1){
        twritef("Unable to resolve %s\n", argv[1]);
        return 1;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = ip == LOOPBACK_IP ? htonl(ip) : ip;
    dest_addr.sin_port = htons(atoi(argv[2]));
    dest_addr.sin_family = AF_INET;

    int con = kernel_connect(socket, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
    if(con < 0) {
        twritef("Unable to connect to %s:%s\n", argv[1], argv[2]);
        return 1;
    }

    twritef("Connected to %s:%s\n", argv[1], argv[2]);
    $process->current->term->ops->commit($process->current->term); 

    char socket_str[10] = {0};
    itoa(socket->socket, socket_str);

    pid_t reader = pcb_create_kthread(__tcp_reader, "tcp_reader", 1, socket_str);
    
    int ret;
    char* buffer = kalloc(1024);
    while(1){
        ret = $process->current->term->ops->scan($process->current->term, buffer, 255);
        if(ret < 0){
            break;
        }
        ret = kernel_send(socket, buffer, strlen(buffer), 0);
        if(ret < 0){
            twritef("Unable to send TCP packet\n");
            break;
        }
    }

    pcb_kill(reader);

    kernel_sock_close(socket);
    return 0;
}
EXPORT_KSYMBOL(tcp);

static int conf(int argc, char *argv[])
{
    int ret;

    if(IS_AUTHORIZED(ADMIN_FULL_ACCESS) == 0) {
        twritef("You are not authorized to use this command\n");
        return 1;
    }

    if(argc < 2) {
        twritef("Usage: conf <load, get, list>\n");
        return 1;
    }

    if(strcmp(argv[1], "list") == 0) {
        config_list();
    } else if(strcmp(argv[1], "load") == 0) {
        if(argc < 3) {
            twritef("Usage: conf load <filename>\n");
            return 1;
        }
        ret = kernel_config_load(argv[2]);
        if(ret < 0) {
            twritef("Failed to load config file: %d\n", ret);
            return 1;
        }
        twritef("Loaded config file %s\n", argv[2]);

    } else if(strcmp(argv[1], "get") == 0) {
        if(argc < 4) {
            twritef("Usage: conf get <section> <value>\n");
            return 1;
        }
        char* value = kernel_config_get_value(argv[2], argv[3]);
        if(value == NULL) {
            twritef("Value not found\n");
            return 1;
        }
        twritef("%s\n", value);
    } else {
        twritef("Usage: conf <load, get, list>\n");
        return 1;
    }

    return 0;
}
EXPORT_KSYMBOL(conf);

static int clear(){
    $process->current->term->ops->reset($process->current->term);
    return 0;
}
EXPORT_KSYMBOL(clear);

static panic(int argc, char *argv[])
{
    if(argc < 2) {
        twritef("Usage: panic <message>\n");
        return 1;
    }

    kernel_panic(argv[1]);
    return 0;
}
EXPORT_KSYMBOL(panic);

static int services(int argc, char *argv[])
{
    if(argc < 2) {
        twritef("Usage: services <list, start, stop>\n");
        return 1;
    }

    if(strcmp(argv[1], "list") == 0) {
        twritef("Services:\n");
        
        twritef("UserManager:    %s\n", $services->usermanager != NULL ? "running" : "stopped");
        twritef("Scheduler:      %s\n", $services->scheduler != NULL ? "running" : "stopped");
        twritef("NetworkManager: %s\n", $services->networking != NULL ? "running" : "stopped");
        return 0;   
    }

    if(strcmp(argv[1], "start") == 0) {
        
        if(IS_AUTHORIZED(ADMIN_FULL_ACCESS) == 0) {
            twritef("You are not authorized to use this command\n");
            return 1;
        }

        if(argc < 3) {
            twritef("Usage: services start <service>\n");
            return 1;
        }


        return 1;
    }

    return 0;
}
EXPORT_KSYMBOL(services);

static int reboot(){
    twritef("Rebooting...\n");
    system_reboot();
    return 0;
}
EXPORT_KSYMBOL(reboot);

/* Process management */

/* System management */

/* Network management */

/* Administration management */

/* package? */