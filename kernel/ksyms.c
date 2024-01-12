/**
 * @file ksyms.c
 * @author Joe Bayer (joexbayer)
 * @brief Kernel symbol table.
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <libc.h>
#include <ksyms.h>
#include <serial.h>
#include <terminal.h>
#include <assert.h>
#include <fs/fs.h>
#include <math.h>
#include <vbe.h>

#define KSYMS_MAX_SYMBOLS 100
#define KSYMS_MAX_DEPTH 100
#define KSYMS_MAX_SYMBOL_LENGTH 25

#define MAX_SYMBOLS 800

/* kernel symbol table structure. */
static struct kernel_symbols {
    struct symbol_entry {
        char name[KSYMS_MAX_SYMBOL_LENGTH];
        uintptr_t addr;
    } symtable[KSYMS_MAX_SYMBOLS];
    
    int num_symbols;
} __ksyms = {
    .num_symbols = 0
};

static struct symbols {
    struct entry {
        char name[50];
        uintptr_t addr;
    } symtable[MAX_SYMBOLS];
    uintptr_t min;
    uintptr_t max;
    int num_symbols;
};
static struct symbols* __symbols;

static int __init_symbols(void)
{
    __symbols = create(struct symbols);
    if(__symbols == NULL) return -1;

    __symbols->num_symbols = 0;
    __symbols->min = (uintptr_t)-1;
    __symbols->max = 0;
    return 0;
}

static int __load_symbols(void)
{
    struct filesystem* fs = fs_get();
    if(fs == NULL) return -1;

    struct file* file = fs->ops->open(fs, "/sysutil/symbols.map", FS_FILE_FLAG_READ);
    if(file == NULL) return -2;

    char* buf = (char*) kalloc(MAX_SYMBOLS*50);
    if(buf == NULL) return -3;

    int read = fs->ops->read(fs, file, buf, MAX_SYMBOLS*50);
    if(read < 0){
        kfree(buf);
        return -4;
    }

    /* 000xxxxx symbol\n */
    int i = 0;
    int j = 0;
    while(i < read){
        if(buf[i] == ' '){
            __symbols->symtable[__symbols->num_symbols].addr = (uintptr_t) htoi(&buf[j]);
            __symbols->symtable[__symbols->num_symbols].name[0] = '\0';

            if(__symbols->symtable[__symbols->num_symbols].addr < __symbols->min) __symbols->min = __symbols->symtable[__symbols->num_symbols].addr;
            if(__symbols->symtable[__symbols->num_symbols].addr > __symbols->max) __symbols->max = __symbols->symtable[__symbols->num_symbols].addr;

            j = i + 1;
        } else if(buf[i] == '\n'){
            memcpy(__symbols->symtable[__symbols->num_symbols].name, &buf[j], i - j);
            __symbols->symtable[__symbols->num_symbols].name[i - j] = '\0';
            __symbols->num_symbols++;
            if(__symbols->num_symbols >= MAX_SYMBOLS) break;
            j = i + 1;
        }
        i++;
    }
    dbgprintf("Loaded %d symbols\n", __symbols->num_symbols);

    kfree(buf);
    return 0;
}

int ksyms_init(void)
{
    if(__init_symbols() < 0) return -1;

    switch(__load_symbols()){
        case -1:
            dbgprintf("Error: no filesystem available\n");
            return -1;
        case -2:
            dbgprintf("Error: could not open symbols.map\n");
            return -2;
        case -3:
            dbgprintf("Error: could not allocate memory for symbols.map\n");
            return -3;
        case -4:
            dbgprintf("Error: could not read symbols.map\n");
            return -4;
    }

    return 0;
}

/**
 * @brief Adds a symbol to the kernel symbol table.
 * This function is usually called by the EXPORT_KSYMBOL macro.
 * But can be called directly at runtime if needed.
 * @param name name of the symbol
 * @param addr address of the symbol
 */
void ksyms_add_symbol(const char* name, uintptr_t addr) {
    
    assert(strlen(name) < KSYMS_MAX_SYMBOL_LENGTH);

    if (__ksyms.num_symbols < KSYMS_MAX_SYMBOLS) {
        memcpy(__ksyms.symtable[__ksyms.num_symbols].name, name, strlen(name));
        __ksyms.symtable[__ksyms.num_symbols].addr = addr;
        __ksyms.num_symbols++;
    } else {
        twritef("Error: symbol table full\n");
    }
}

void ksyms_list(void)
{
    for (int i = 0; i < __ksyms.num_symbols; i++) {
        twritef("%s\n", __ksyms.symtable[i].name);
    }
}

/**
 * @brief Checks if there exists a kernel symbol with the given name.
 * 
 * @param name name to resolve
 * @return uintptr_t function pointer, NULL on error.
 */
uintptr_t ksyms_resolve_symbol(const char* name)
{
    ERR_ON_NULL(name);
    for (int i = 0; i < __ksyms.num_symbols; i++) {
        int ksym_namelen = strlen(__ksyms.symtable[i].name);
        int namelen = strlen(name);
        if(memcmp(name, __ksyms.symtable[i].name, namelen) == 0 && ksym_namelen == namelen){
            return __ksyms.symtable[i].addr;
        }
    }

    return 0;
}

#define MAX_BACKTRACE_DEPTH 100

void __backtrace_from(uintptr_t* ebp)
{
    uintptr_t stack[MAX_BACKTRACE_DEPTH] = {0};
    int depth = 0;
    while (depth < MAX_BACKTRACE_DEPTH && ebp) {
        uintptr_t ret_addr = *(ebp + 1);
        stack[depth++] = ret_addr;
        
        if (ret_addr == 0) break;
        if (ret_addr > 0x100000) break; // TODO: fix this (it's a hack to prevent backtracing into user space)
        ebp = (uintptr_t *)*ebp;
    }

    for (int i = 0; i < depth; i++) {
        uintptr_t addr = stack[i];
        
        // Find the closest symbol
        int found = 0;
        for (int j = 0; j < __symbols->num_symbols; j++) {
            if (__symbols->symtable[j].addr <= addr && (j == __symbols->num_symbols - 1 || __symbols->symtable[j + 1].addr > addr)) {
                
                dbgprintf("%s: 0x%x - 0x%x = 0x%x\n", 
                    __symbols->symtable[j].name, 
                    addr, 
                    __symbols->symtable[j].addr, 
                    addr - __symbols->symtable[j].addr);

                /*vesa_printf((uint8_t*)vbe_info->framebuffer, 0, i*8 + 100, 0, "%s: 0x%x - 0x%x = 0x%x\n", 
                    __symbols->symtable[j].name, 
                    addr, 
                    __symbols->symtable[j].addr, 
                    addr - __symbols->symtable[j].addr);*/
                found = 1;
                break;
            }
        }

        if (!found) {
            dbgprintf("0x%x\n", addr);
        }
    }
}

void backtrace() {
    uintptr_t *ebp  = __builtin_frame_address(0);
    __backtrace_from(ebp);
}
EXPORT_KSYMBOL(backtrace);