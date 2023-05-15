/**
 * @file interpreter.c
 * @author Joe Bayer (joexbayer)
 * @brief main file for the minmial C interpreter
 * @version 0.1
 * @date 2023-05-14
 * @see https://github.com/rswier/c4
 * @see https://github.com/lotabout/write-a-C-interpreter
 * @copyright Copyright (c) 2023
 * 
 */

#include "vm.h"
#include "lex.h"
#include "interpreter.h"
#include <kthreads.h>

struct vm vm;
inode_t fd;
static char *src, *old_src;
int i = 0;

int interpreter(int argc, char **argv)
{
    vm_init(&vm);
    lex_init();
    
    argv++;

    // read the source file
    DEBUG_PRINT("Reading hello.c\n");
    if ((fd = fs_open("hello.c")) < 0) {
        twritef("could not open(hello.c)\n");
        return -1;
    }

    if (!(src = old_src = kalloc(POOLSIZE))) {
        twritef("could not malloc(%d) for source area\n", POOLSIZE);
        return -1;
    }
    // read the source file
    if ((i = fs_read(fd, src, POOLSIZE)) <= 0) {
        twritef("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // add EOF character
    DEBUG_PRINT("%s\n", src);
    fs_close(fd);

    DEBUG_PRINT("Lexing\n");
    void* entry = program(vm.text, vm.data, src);
    DEBUG_PRINT("Lexing [done]\n");

    vm.pc = entry;
    DEBUG_PRINT("Main entry %x\n", vm.pc);

    vm_setup_stack(&vm ,argc, argv);

    eval(&vm);

    vm_free(&vm);

    kfree(src);
}
EXPORT_KTHREAD(interpreter);