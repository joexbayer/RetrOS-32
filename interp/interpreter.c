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

#define DEFAULT_OUT "bytecode.o"

int rc(int argc, char **argv)
{
    inode_t fd;
    int i;
    char *src, *old_src;
    struct vm vm;

    if(argc == 1){
        twritef("usage: as <file>\n");
        return -1;
    }

    argv++;

    if ((fd = fs_open(*argv, 0)) <= 0) {
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
        return -1;
    }
    dbgprintf("read() returned %d\n", i);
    src[i+1] = 0; // add EOF character
    DEBUG_PRINT("%s\n", src);
    fs_close(fd);

    unsigned char* dec = kalloc(i*2);
    int dec_sz = 0;
    run_length_decode(src, i, dec, &dec_sz);

    struct lexed_file* lexd = (struct lexed_file*) dec;
    dbgprintf("%d : %d : %d\n", lexd->datasize, lexd->entry, lexd->textsize);
    dec += sizeof(struct lexed_file);

    int* text = (int*) dec;
    dec += lexd->textsize;

    char* data = (char*) dec;

    vm_setup(&vm, text, data);

    vm.pc = (int)vm.text+lexd->entry;

    vm_setup_stack(&vm ,argc, argv);

    eval(&vm, 0);

    //vm_free(&vm);

    kfree(vm.stack);
    kfree(dec);
    kfree(src);
}
EXPORT_KSYMBOL(rc);

int as(int argc, char **argv)
{
    inode_t fd;
    int i;
    char *src, *old_src;

    if(argc == 1){
        twritef("usage: as <file>\n");
        return -1;
    }

    argv++;

    if ((fd = fs_open(*argv, 0)) <= 0) {
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
        return -1;
    }
    src[i+1] = 0; // add EOF character
    DEBUG_PRINT("%s\n", src);
    fs_close(fd);

    int* text = kalloc(VM_TEXT_SIZE);
    char* data = kalloc(VM_DATA_SIZE);

    lex_init();
    struct lexed_file lexd = program(text, data, src);
    dbgprintf("%d : %d : %d\n", lexd.datasize, lexd.entry, lexd.textsize);
    if(lexd.entry == 0)
    {
        twritef("%d: %s\n", lex_get_error_line(), lex_get_error());
        return -1;
    }

    if ((fd = fs_open(DEFAULT_OUT, FS_FLAG_CREATE)) <= 0) {
        twritef("could not open(%s)\n", *argv);
        return -1;
    }

    int sz = sizeof(struct lexed_file) + lexd.datasize + lexd.textsize;
    char* buffer = kalloc(sz);
    char* original_buffer = buffer;

    memcpy(buffer, &lexd, sizeof(struct lexed_file));
    buffer += sizeof(struct lexed_file);
    memcpy(buffer, text, lexd.textsize);
    buffer += lexd.textsize;
    memcpy(buffer, data, lexd.datasize);

    unsigned char* enc = kalloc(sz*2);
    int enc_sz = 0;

    run_length_encode(original_buffer, sz, enc, &enc_sz);

    fs_write(fd, enc, enc_sz);

    fs_close(fd);

    kfree(enc);
    kfree(text);
    kfree(data);
    kfree(src);

    twritef("Assembled file %s to %s\n", *argv, DEFAULT_OUT);
}
EXPORT_KSYMBOL(as);

int cc(int argc, char **argv)
{
    struct vm vm;
    inode_t fd;
    static char *src, *old_src;
    int i = 0;

    if(argc == 1){
        twritef("usage: cc [-s] <file>\n");
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
    if ((fd = fs_open(*argv, 0)) < 0) {
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
    struct lexed_file lexd = program(vm.text, vm.data, src);
    if(lexd.entry == 0)
    {
        twritef("%d: %s\n", lex_get_error_line(), lex_get_error());
        return -1;
    }
    DEBUG_PRINT("Lexing [done]\n");

    vm.pc = (int)vm.text+lexd.entry;
    DEBUG_PRINT("Main entry %x\n", vm.pc);

    vm_setup_stack(&vm ,argc, argv);

    eval(&vm, assembly);

    vm_free(&vm);

    kfree(src);
}
EXPORT_KSYMBOL(cc);