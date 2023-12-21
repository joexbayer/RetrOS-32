/**
 * @file vm.c
 * @author Joe Bayer (joexbayer)
 * @brief Virtual Machine for the minmial C interpreter
 * @version 0.1
 * @date 2023-05-14
 * @see https://github.com/rswier/c4
 * @see https://github.com/lotabout/write-a-C-interpreter
 * @copyright Copyright (c) 2023
 * 
 */
#include "interpreter.h"
#include "vm.h"

const char* optcodes[] = {
    "LEA", "IMM", "JMP", "CALL", "JZ", "JNZ", "ENT", "ADJ", "LEV", "LI",
    "LC", "SI", "SC", "PUSH", "OR", "XOR", "AND", "EQ", "NE", "LT",
    "GT", "LE", "GE", "SHL", "SHR", "ADD", "SUB", "MUL", "DIV", "MOD",
    "OPEN", "READ", "CLOS", "PRTF", "MALC", "MSET", "MCMP", "EXIT", "FREE"
};


void vm_print(struct vm* vm)
{
    twritef("__.--@ / VM \\ @--.__\n");
    twritef("  Stack: 0x%x\n", vm->stack);
    twritef("  Data:  0x%x\n", vm->data);
    twritef("  Text:  0x%x\n", vm->text);
    twritef("  ----------------\n");
    twritef("  AX: 0x%x\n", vm->ax);
    twritef("  PC: 0x%x\n", vm->pc);
    twritef("  SP: 0x%x\n", vm->sp);
    twritef("__.--@ / VM \\ @--.__\n");
    DEBUG_PRINT("  Stack:\n");
    //hexdump(vm->text, POOLSIZE);
    
}

int eval(struct vm* vm, int assembly)
{
    //vm_print(vm);

    int op, *tmp;
    while (1) {
        op = *vm->pc++; /* get next operation code */
        switch (op) {
            case IMM:
                vm->ax = *vm->pc++; /* load immediate value to vm->ax */
                break;
            case LC:
                vm->ax = *(char *)vm->ax; /* load character to vm->ax, address in vm->ax */
                break;
            case LI:
                vm->ax = *(int *)vm->ax; /* load integer to vm->ax, address in vm->ax */
                break;
            case SC:
                vm->ax = *(char *)*vm->sp++ = vm->ax; /* save character to address, value in vm->ax, address on stack */
                break;
            case SI:
                *(int *)*vm->sp++ = vm->ax; /* save integer to address, value in vm->ax, address on stack */
                break;
            case PUSH:
                *--vm->sp = vm->ax; /* push the value of vm->ax onto the stack */
                break;
            case JMP:
                vm->pc = (int *)*vm->pc; /* jump to the address */
                break;
            case JZ:
                vm->pc = vm->ax ? vm->pc + 1 : (int *)*vm->pc; /* jump if vm->ax is zero */
                break;
            case JNZ:
                vm->pc = vm->ax ? (int *)*vm->pc : vm->pc + 1; /* jump if vm->ax is not zero */
                break;
            case CALL:
                *--vm->sp = (int)(vm->pc + 1); /* Store next program counter on stack */
                vm->pc = (int *)*vm->pc; /* call subroutine */
                break;
            case ENT:
                *--vm->sp = (int)vm->bp;
                vm->bp = vm->sp;
                vm->sp = vm->sp - *vm->pc++; /* make new stack frame */
                break;
            case ADJ:
                vm->sp = vm->sp + *vm->pc++; /* add evm->sp, <size> */
                break;
            case LEV:
                vm->sp = vm->bp;
                vm->bp = (int *)*vm->sp++;
                vm->pc = (int *)*vm->sp++; /* restore call frame and vm->pc */
                break;
            case LEA:
                vm->ax = (int)(vm->bp + *vm->pc++); /* load address for arguments. */
                break;
            case OR:
                vm->ax = *vm->sp++ | vm->ax;
                break;
            case XOR:
                vm->ax = *vm->sp++ ^ vm->ax;
                break;
            case AND:
                vm->ax = *vm->sp++ & vm->ax;
                break;
            case EQ:
                vm->ax = *vm->sp++ == vm->ax;
                break;
            case NE:
                vm->ax = *vm->sp++ != vm->ax;
                break;
            case LT:
                vm->ax = *vm->sp++ < vm->ax;
                break;
            case LE:
                vm->ax = *vm->sp++ <= vm->ax;
                break;
            case GT:
                vm->ax = *vm->sp++ > vm->ax;
                break;
            case GE:
                vm->ax = *vm->sp++ >= vm->ax;
                break;
            case SHL:
                vm->ax = *vm->sp++ << vm->ax;
                break;
            case SHR:
                vm->ax = *vm->sp++ >> vm->ax;
                break;
            case ADD:
                vm->ax = *vm->sp++ + vm->ax;
                break;
            case SUB:
                vm->ax = *vm->sp++ - vm->ax;
                break;
            case MUL:
                vm->ax = *vm->sp++ * vm->ax;
                break;
            case DIV:
                vm->ax = *vm->sp++ / vm->ax;
                break;
            case MOD:
                vm->ax = *vm->sp++ % vm->ax;
                break;
            case EXIT:
                twritef("exit(%d)\n", *vm->sp);
                return *vm->sp;
            case OPEN:
                vm->ax = ext_open((char*)vm->sp[1], 0);
                break;
            case CLOS:
                vm->ax = 0;
                ext_close(*vm->sp);
                break;
            case READ:
                vm->ax = ext_read(vm->sp[2], (char *)vm->sp[1], *vm->sp);
                break;
            case PRTF:
                tmp = vm->sp + vm->pc[1];
                vm->ax = 0;
                twritef((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]);
                break;
            case MALC:
                vm->ax = (int)kalloc(*vm->sp);
                break;
            case MSET:
                vm->ax = (int)memset((char *)vm->sp[2], vm->sp[1], *vm->sp);
                break;
            case MCMP:
                vm->ax = memcmp((char *)vm->sp[2], (char *)vm->sp[1], *vm->sp);
                break;
            case FREE:
                kfree((void *)*vm->sp);
                break;
            default:
                twritef("unknown instruction:%d\n", op);
                return -1;
        }
        if(assembly) twritef("%s, AX: %d\n", optcodes[op], vm->ax);
    }
    terminal_commit();
    return 0;
}

void vm_setup(struct vm* vm, int* text, char* data)
{
    vm->text = text;
    vm->data = data;
    vm->stack = kalloc(VM_STACK_SIZE);
    vm->old_text = vm->text;
    
    vm->bp = vm->sp = (int *)((int)vm->stack + VM_STACK_SIZE-2);
    vm->ax = 0;
}

void vm_init(struct vm* vm)
{
    /* allocate memory for virtual machine */

    vm->text = kalloc(VM_TEXT_SIZE);
    vm->data = kalloc(VM_DATA_SIZE);
    vm_setup(vm, vm->text, vm->data);
}

void vm_free(struct vm* vm)
{
    DEBUG_PRINT("Cleaning VM:\nText: 0x%x\nData: 0x%x\nStack: 0x%x\n", vm->text, vm->data, vm->stack);
    kfree(vm->text);
    kfree(vm->data);
    kfree(vm->stack);
}

void vm_setup_stack(struct vm* vm, int argc, char* argv[])
{
    int tmp;
    /* setup stack */
    vm->sp = (int *)((int)vm->stack + VM_STACK_SIZE-2);
    *--vm->sp = EXIT; /* call exit if main returns */
    *--vm->sp = PUSH;
    tmp = vm->sp;
    *--vm->sp = argc;
    *--vm->sp = (int)argv;
    *--vm->sp = (int)tmp;
}