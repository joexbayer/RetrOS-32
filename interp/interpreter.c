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
#include <ksyms.h>
#include <stdint.h>

struct vm vm;
inode_t fd;
static char *src, *old_src;
int i = 0;

int cc(int argc, char **argv)
{
    if(argc == 1){
        twritef("usage: cc <file>\n");
        return -1;
    }

    int assembly = 0;

    char* optarg = NULL;
    int option;
    while ((option = getopt(argc, argv, "s", &optarg)) != -1) {
        switch (option) {
            case 's':
                assembly = 1;
                break;
            case '?':
                twritef("Invalid option: %c\n", optarg[0]);
                return;
            case ':':
                twritef("Option requires an argument: %c\n", optarg[0]);
                return;
            default:
                twritef("Unknown argument %c\n", option);
                break;
        }
    }
    if(assembly)
        argv++;

    vm_init(&vm);
    lex_init();
    
    argv++;

    // read the source file
    DEBUG_PRINT("Reading (%s)\n", *argv);
    if ((fd = fs_open(*argv)) < 0) {
        twritef("could not open(%s)\n", *argv);
        return -1;
    }

    if (!(src = old_src = kalloc(POOLSIZE))) {
        twritef("could not malloc(%d) for source area\n", POOLSIZE);
        return -1;
    }
    // read the source file
    if ((i = fs_read(fd, src, MAX_FILE_SIZE)) <= 0) {
        twritef("read() returned %d\n", i);
        kfree(src);
        vm_free(&vm);
        return -1;
    }
    src[i+1] = 0; // add EOF character
    DEBUG_PRINT("%s\n", src);
    fs_close(fd);

    DEBUG_PRINT("Lexing\n");
    void* entry = program(vm.text, vm.data, src);
    if(entry == NULL)
    {
        twritef("%d: %s\n", lex_get_error_line(), lex_get_error());
        return -1;
    }
    DEBUG_PRINT("Lexing [done]\n");

    vm.pc = entry;
    DEBUG_PRINT("Main entry %x\n", vm.pc);

    vm_setup_stack(&vm ,argc, argv);

    eval(&vm, assembly);

    vm_free(&vm);

    kfree(src);
}
EXPORT_KSYMBOL(cc);