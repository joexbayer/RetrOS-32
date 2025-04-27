/**
 * @file kutils.c
 * @author Joe Bayer (joexbayer)
 * @brief Kernel utilities.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <libc.h>
#include <memory.h>
#include <serial.h>
#include <ksyms.h>
#include <terminal.h>
#include <gfx/gfxlib.h>
#include <vbe.h>
#include <pcb.h>
#include <arch/io.h>
#include <script.h>
#include <kutils.h>
#include <syscalls.h>
#include <syscall_helper.h>
#include <errors.h>

static char *units[] = {"b ", "kb", "mb"};

void system_reboot()
{
    ENTER_CRITICAL();
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inportb(0x64);
    outportb(0x64, 0xFE);
    HLT();
}

/* Function to shut down the system */
void system_shutdown()
{
    ENTER_CRITICAL();
    outportw(0x604, 0x2000);

    /* Halt the CPU as a fail-safe in case shutdown does not initiate */
    HLT();
}

/* Function to align a given size to the size of a void* */
int align_to_pointer_size(int size)
{
    /* Calculate the alignment requirement */
    int alignment = sizeof(void*);

    /* Align the size */
    int aligned_size = (size + alignment - 1) & ~(alignment - 1);

    return aligned_size;
}

int exec_cmd(char* str)
{
    if(str == NULL || *str == 0) return -1;

    struct args args = {
        .argc = 0
    };

    for (int i = 0; i < 10; i++){
        args.argv[i] = args.data[i];
    }
    
	args.argc = parse_arguments(str, args.data);
	if(args.argc == 0) return -1;

	void (*ptr)(int argc, char* argv[]) = (void (*)(int argc, char* argv[])) ksyms_resolve_symbol(args.argv[0]);
	if(ptr == NULL){
		return -1;
	}

    dbgprintf("Executing %s\n", args.argv[0]);
    /* execute command */
	ptr(args.argc, args.argv);

	gfx_commit();

	return 0;
}
EXPORT_KSYMBOL(exec_cmd);

/**
 * @brief Converts a amount of bytes to a human readable format 
 * 
 * @param bytes amount of bytes
 * @return struct unit 
 */
struct unit calculate_size_unit(int bytes)
{
    int index = 0;
    double size = bytes;

    while (size >= 1024 && index < 2) {
        size /= 1024;
        index++;
    }

    struct unit unit = {
        .size = size,
        .unit = units[index]
    };

    return unit;
}

unsigned int advanced_hash(char *input)
{
    unsigned int hash = 0;
    int c;

    /* Loop through each character in the password */
    while ((c = *input++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

void kernel_panic(const char* reason)
{
    ENTER_CRITICAL();
    
    dbgprintf("KERNEL PANIC: %s\n", reason);
    //backtrace();
    /* fill screen with blue */
    memset((uint8_t*)vbe_info->framebuffer, 0x01, vbe_info->pitch * vbe_info->height);

    const char* message = "KERNEL PANIC";
    int message_len = strlen(message);

    for (int i = 0; i < message_len; i++){
        vesa_put_char16((uint8_t*)vbe_info->framebuffer, message[i], 16+(i*16), vbe_info->height/3 - 24, 15);
    }
    
    struct pcb* pcb = $process->current;
    vesa_printf((uint8_t*)vbe_info->framebuffer, 16, vbe_info->height/3, 15, "A critical error has occurred and your system is unable to continue operating.\nThe cause of this failure appears to be an essential system component.\n\nReason:\n%s\n\n###### PCB ######\npid: %d\nname: %s\nesp: 0x%x\nebp: 0x%x\nkesp: 0x%x\nkebp: 0x%x\neip: 0x%x\nstate: %s\nstack limit: 0x%x\nstack size: 0x%x (0x%x - 0x%x)\nPage Directory: 0x%x\nCS: %d\nDS:%d\n\n\nPlease power off and restart your device.\nRestarting may resolve the issue if it was caused by a temporary problem.\nIf this screen appears again after rebooting, it indicates a more serious issue.",
    reason, pcb->pid, pcb->name, pcb->ctx.esp, pcb->ctx.ebp, pcb->kesp, pcb->kebp, pcb->ctx.eip, pcb_status[pcb->state], pcb->stackptr, (int)((pcb->stackptr+0x2000-1) - pcb->ctx.esp), (pcb->stackptr+0x2000-1), pcb->ctx.esp,  pcb->page_dir, pcb->cs, pcb->ds);

    PANIC();
}

int kref_init(struct kref* ref)
{
    ref->refs = 0;
    ref->spinlock = 0;

    return 0;
}

int kref_get(struct kref* ref)
{
    spin_lock(&ref->spinlock);

    ref->refs++;

    spin_unlock(&ref->spinlock);

    return ref->refs;
}

int kref_put(struct kref* ref)
{
    spin_lock(&ref->spinlock);

    ref->refs--;

    spin_unlock(&ref->spinlock);

    return ref->refs;
}

/* TODO: Move out of this file... */
int script_parse(char* str)
{
    char* start = str;
    int line = 0, ret;

    if(*str == 0){
        return -1;
    }

    /* This assumes that the given string is \0 terminated. */
    do {
        if(*str == '\n'){
            *str = 0;
            
            ret = exec_cmd(start);
            if(ret < 0){
                twritef("script: error on '%s' line %d\n", start, line);
                return -1;
            }
        
            line++;
            start = str+1;
        }
        str++;
    } while (*str != 0);
    
    /* Try to execute the last line in case it ended with a \0 */
    ret = exec_cmd(start);
    if(ret < 0){
        twritef("script: error on '%s' line %d\n", start, line);
        return -1;
    }
        

    return 0;
}

/* Get the size of the sample function */
int kfunc_size(void (*func)()) {
    unsigned long func_addr = (unsigned long)func;
    uint8_t *ptr = (uint8_t *)func_addr;
    int size = 0;
    while (ptr[size] == 0xC3 && ptr[size-1] != 0xC9 ) { // Look for the 'ret' instruction (0xC3)
        size++;
    }
    return size + 1;
}

void disassemble(uint8_t *code_buffer, int size, uint32_t* base_address) {
    twritef("Not implemented\n");
    return;
}


int kregisters(struct registers_dump* regs){
    /* Fill in regs */
    asm volatile("movl %%eax, %0" : "=r"(regs->eax));
    asm volatile("movl %%ebx, %0" : "=r"(regs->ebx));
    asm volatile("movl %%ecx, %0" : "=r"(regs->ecx));
    asm volatile("movl %%edx, %0" : "=r"(regs->edx));
    asm volatile("movl %%esi, %0" : "=r"(regs->esi));
    asm volatile("movl %%edi, %0" : "=r"(regs->edi));
    asm volatile("movl %%ebp, %0" : "=r"(regs->ebp));
    asm volatile("movl %%esp, %0" : "=r"(regs->esp));
    asm volatile("call 1f\n1: pop %0" : "=r"(regs->eip));
    asm volatile("pushf\npop %0" : "=r"(regs->eflags));
    asm volatile("movl %%cr0, %0" : "=r"(regs->cr0));
    asm volatile("movl %%cr2, %0" : "=r"(regs->cr2));
    asm volatile("movl %%cr3, %0" : "=r"(regs->cr3));
    asm volatile("movl %%cr4, %0" : "=r"(regs->cr4));
    asm volatile("movl %%gs, %0" : "=r"(regs->gs));

    return 0;
}