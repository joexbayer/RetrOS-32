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
    struct args args = {
        .argc = 0
    };

    for (int i = 0; i < 10; i++){
        args.argv[i] = args.data[i];
    }
    
	args.argc = parse_arguments(str, args.data);
	if(args.argc == 0) return -1;

    for (int i = 0; i < args.argc; i++){
        dbgprintf("%d: %s\n", args.argc, args.argv[i]);
    }

	void (*ptr)(int argc, char* argv[]) = (void (*)(int argc, char* argv[])) ksyms_resolve_symbol(args.argv[0]);
	if(ptr == NULL){
		return -1;
	}

    dbgprintf("Executing %s\n", args.argv[0]);
    /* execute command */
	ptr(args.argc, args.argv);
    dbgprintf("Done executing %s\n", args.argv[0]);

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
    
    /* Try to execute the last line incase it ended with a \0 */
    ret = exec_cmd(start);
    if(ret < 0){
        twritef("script: error on '%s' line %d\n", start, line);
        return -1;
    }
        

    return 0;
}

/* Get the size of the sample function */
int kfunc_size(void (*func)()) {
    uint8_t *ptr = (uint8_t *)func;
    int size = 0;
    while (ptr[size] != 0xC3) { // Look for the 'ret' instruction (0xC3)
        size++;
    }
    return size + 1;
}

int print_instruction(uint8_t *code_buffer, int offset, uint32_t *base_address) {
    uint8_t opcode = code_buffer[offset];
    int instr_length = 1;

    switch (opcode) {
        case 0xf3:
            if (code_buffer[offset + 1] == 0x0f && code_buffer[offset + 2] == 0x1e && code_buffer[offset + 3] == 0xfb) {
                twritef("f3 0f 1e fb          endbr32\n");
                return 4;
            }
            break;
        case 0x90:
            twritef("90                   nop\n");
            return 1;
        case 0x55:
            twritef("55                   push %%ebp\n");
            return 1;
        case 0x89:
            if (code_buffer[offset + 1] == 0xe5) {
                twritef("89 e5                mov %%esp, %%ebp\n");
                return 2;
            }
            if (code_buffer[offset + 1] == 0xd8) {
                twritef("89 d8                mov %%ebx, %%eax\n");
                return 2;
            }
            break;
        /* Add */
        case 0x83:
            if (code_buffer[offset + 1] == 0xec) {
                twritef("83 ec %x             sub $0x%x, %%esp\n", code_buffer[offset + 2], code_buffer[offset + 2]);
                return 3;
            }
            if (code_buffer[offset + 1] == 0xc0) {
                twritef("83 c0 %x             add $0x%x, %%eax\n", code_buffer[offset + 2], code_buffer[offset + 2]);
                return 3;
            }
            if (code_buffer[offset + 1] == 0x7d && code_buffer[offset + 2] == 0x08) {
                twritef("83 7d 08 01          cmp $0x1, 0x8(%%ebp)\n");
                return 4;
            }
            if (code_buffer[offset + 1] == 0xc4) {
                twritef("83 c4 %x             add $0x%x, %%esp\n", code_buffer[offset + 2], code_buffer[offset + 2]);
                return 3;
            }
            if (code_buffer[offset + 1] == 0xc0 && code_buffer[offset + 2] == 0x01) {
                twritef("83 c0 01             add $0x1, %%eax\n");
                return 3;
            }
            if (code_buffer[offset + 1] == 0xc1 && code_buffer[offset + 2] == 0x01) {
                twritef("83 c1 01             add $0x1, %%ecx\n");
                return 2;
            }
            break;
        case 0x7e:
            twritef("7e %x                jle 0x%x\n", code_buffer[offset + 1], offset + 2 + (int8_t)code_buffer[offset + 1]);
            return 2;
        case 0xa1:
            twritef("a1 %x %x %x %x       mov eax, 0x%x\n",
                code_buffer[offset + 1], code_buffer[offset + 2], code_buffer[offset + 3], code_buffer[offset + 4],
                *((uint32_t *)(code_buffer + offset + 1)));
            return 5;
        case 0x8b:
            if (code_buffer[offset + 1] == 0x00) {
                twritef("8b 00                mov (%%eax), %%eax\n");
                return 2;
            }
            if (code_buffer[offset + 1] == 0x80 && code_buffer[offset + 2] == 0xf2 && code_buffer[offset + 3] == 0x00 && code_buffer[offset + 4] == 0x00 && code_buffer[offset + 5] == 0x00) {
                twritef("8b 80 f2 00 00 00    mov 0xf2(%%eax), %%eax\n");
                return 6;
            }
            if (code_buffer[offset + 1] == 0x4d && code_buffer[offset + 2] == 0x0c) {
                twritef("8b 4d 0c             mov 0xc(%%ebp), %%ecx\n");
                return 3;
            }
            if (code_buffer[offset + 1] == 0x50) {
                twritef("8b 50 1c             mov 0x1c(%%eax), %%edx\n");
                return 3;
            }
            break;
        case 0x85:
            if (code_buffer[offset + 1] == 0xc0) {
                twritef("85 c0                test %%eax, %%eax\n");
                return 2;
            }
            break;
        case 0x84:
            if (code_buffer[offset + 1] == 0xd2) {
                twritef("84 d2                test %%dl %%dl\n");
                return 2;
            }
            break;
        case 0x74:
            twritef("74 %x                je 0x%x\n", code_buffer[offset + 1], offset + 2 + (int8_t)code_buffer[offset + 1]);
            return 2;
        case 0xc7:
            if (code_buffer[offset + 1] == 0x45) {
                int8_t disp = code_buffer[offset + 2];
                twritef("c7 45 %x %x %x %x %x       movl $0x%x, 0x%x(%%ebp)\n", disp,
                    code_buffer[offset + 3], code_buffer[offset + 4], code_buffer[offset + 5], code_buffer[offset + 6],
                    *((uint32_t *)(code_buffer + offset + 3)), disp);
                return 7;
            }
            break;
        case 0xff:
            if (code_buffer[offset + 1] == 0x71 && code_buffer[offset + 2] == 0x04) {
                twritef("ff 71 04             pushl 0x4(%%ecx)\n");
                return 3;
            }
            if (code_buffer[offset + 1] == 0x52) {
                twritef("ff 52 %x             call *0x%x(%%edx)\n", code_buffer[offset + 2], code_buffer[offset + 2]);
                return 3;
            }
            break;
        case 0x68:
            twritef("68 %x %x %x %x       push $0x%x\n",
                code_buffer[offset + 1], code_buffer[offset + 2], code_buffer[offset + 3], code_buffer[offset + 4],
                *((uint32_t *)(code_buffer + offset + 1)));
            return 5;
        case 0x50:
            twritef("50                   push %%eax\n");
            return 1;
        case 0x52:
            twritef("52                   push %%edx\n");
            return 1;
        case 0xc9:
            twritef("c9                   leave\n");
            return 1;
        case 0xc3:
            twritef("c3                   ret\n");
            return 1;
        case 0xb8:
            twritef("b8 %x %x %x %x       mov $0x%x, %%eax\n",
                code_buffer[offset + 1], code_buffer[offset + 2], code_buffer[offset + 3], code_buffer[offset + 4],
                *((uint32_t *)(code_buffer + offset + 1)));
            return 5;
        case 0xb9:
            twritef("b9 %x %x %x %x       mov $0x%x, %%ecx\n",
                code_buffer[offset + 1], code_buffer[offset + 2], code_buffer[offset + 3], code_buffer[offset + 4],
                *((uint32_t *)(code_buffer + offset + 1)));
            return 5;
        case 0xba:
            twritef("ba %x %x %x %x       mov $0x%x, %%edx\n",
                code_buffer[offset + 1], code_buffer[offset + 2], code_buffer[offset + 3], code_buffer[offset + 4],
                *((uint32_t *)(code_buffer + offset + 1)));
            return 5;
        case 0xbb:
            twritef("bb %x %x %x %x       mov $0x%x, %%ebx\n",
                code_buffer[offset + 1], code_buffer[offset + 2], code_buffer[offset + 3], code_buffer[offset + 4],
                *((uint32_t *)(code_buffer + offset + 1)));
            return 5;
        case 0xe9:
            twritef("e9 %x %x %x %x       jmp 0x%x\n",
                code_buffer[offset + 1], code_buffer[offset + 2], code_buffer[offset + 3], code_buffer[offset + 4],
                base_address + offset + 5 + *((int32_t *)(code_buffer + offset + 1)));
            return 5;
        case 0xeb:
            twritef("eb %x                jmp 0x%x\n", code_buffer[offset + 1], base_address + offset + 2 + *((int8_t *)(code_buffer + offset + 1)));
            return 2;
        case 0xe8:
            twritef("e8 %x %x %x %x       call 0x%x\n",
                code_buffer[offset + 1], code_buffer[offset + 2], code_buffer[offset + 3], code_buffer[offset + 4],
                base_address + offset + 5 + *((int32_t *)(code_buffer + offset + 1)));
            return 5;
        case 0x0f:
            dbgprintf("0f\n");
            if (code_buffer[offset + 1] == 0xb6 && code_buffer[offset + 2] == 0x11) {
                twritef("0f b6 11             movzbl (%%ecx), %%edx\n");
                return 3;
            }
            if (code_buffer[offset + 1] == 0xb7 && code_buffer[offset + 2] == 0x52 && code_buffer[offset + 3] == 0x12) {
                twritef("0f b7 52 12          movzwl 0x12(%%edx), %%edx\n");
                return 4;
            }
            break;
        case 0xb6:
            if (code_buffer[offset + 1] == 0x11 && code_buffer[offset + 2] == 0x84 && code_buffer[offset + 3] == 0xd2) {
                twritef("b6 11 84 d2          movzbl %%dl, 0x%x(%%edx)\n", *((uint32_t *)(code_buffer + offset + 3)));
                return 4;
            }
            break;
        case 0x88:
            if (code_buffer[offset + 1] == 0x10) {
                twritef("88 10                mov %%dl, (%%eax)\n");
                return 2;
            }
            break;
        case 0xc6:
            if (code_buffer[offset + 1] == 0x00) {
                twritef("c6 00 00             movb $0x0, (%%eax)\n");
                return 3;
            }
            break;
        default:
            twritef("%x                   UNKNOWN\n", opcode);
            return 1;
    }
    return 1;
}

void disassemble(uint8_t *code_buffer, int size, uint32_t* base_address) {
    int ret;
    int offset = 0;
    while (offset < size) {
        ret = print_instruction(code_buffer, offset, base_address);
        if (ret == -1) {
            break;
        }
        offset += ret;
    }
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
}