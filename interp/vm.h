#ifndef __INTERPRETER_VM_H
#define __INTERPRETER_VM_H

enum {
    LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT
};

struct vm {
    long *text, old_text, *stack;
    char *data;
    long *pc, *bp, *sp, ax, cycle; // virtual machine registers
};

void vm_init(struct vm* vm);
void vm_free(struct vm* vm);
void vm_setup_stack(struct vm* vm, int argc, char* argv[]);
int eval(struct vm* vm);

#endif /* !__INTERPRETER_VM_H */
