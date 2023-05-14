#include "vm.h"
#include "lex.h"
#include "interpreter.h"
#include <stdio.h>

struct vm vm;
int fd;
char *src, *old_src;
int i = 0;

int main(int argc, char **argv)
{
    vm_init(&vm);
    lex_init();
    
    argv++;

    // read the source file
    DEBUG_PRINT("Reading %s\n", *argv);
    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    if (!(src = old_src = malloc(POOLSIZE))) {
        printf("could not malloc(%d) for source area\n", POOLSIZE);
        return -1;
    }
    // read the source file
    if ((i = read(fd, src, POOLSIZE)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // add EOF character
    DEBUG_PRINT("%s\n", src);
    close(fd);

    DEBUG_PRINT("Lexing\n");
    memset(vm.text, 0, 4*1024);
    void* entry = program(vm.text, vm.data, src);
    DEBUG_PRINT("Lexing [done]\n");

    vm.pc = entry;
    DEBUG_PRINT("Main entry %x\n", vm.pc);

    vm_setup_stack(&vm ,argc, argv);

    eval(&vm);

    vm_free(&vm);

    free(old_src);
}