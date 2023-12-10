
#include <util.h>
#include <ksyms.h>
#include <serial.h>
#include <terminal.h>
#include <assert.h>

#define KSYMS_MAX_SYMBOLS 100
#define KSYMS_MAX_DEPTH 100
#define KSYMS_MAX_SYMBOL_LENGTH 25

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
        dbgprintf("Added new symbol %s at 0x%x\n", name, addr);
        __ksyms.num_symbols++;
    } else {
        twrite("Error: symbol table full\n");
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

    return NULL;
}

void __backtrace_find_symbol(uintptr_t* addr)
{
        int best_index = -1;
        uintptr_t best_diff = (uintptr_t)-1; // Max possible value for uintptr_t
        for (int i = 0; i < __ksyms.num_symbols; i++) {
           if (__ksyms.symtable[i].addr <= addr) {
                uintptr_t diff = (uint32_t)addr - (uint32_t)__ksyms.symtable[i].addr;
                if (diff < best_diff) {
                    best_diff = diff;
                    best_index = i;
                }
            }

        }

        if (best_index != -1) {
            dbgprintf("%s: 0x%x - 0x%x = 0x%x\n", 
                      __ksyms.symtable[best_index].name, 
                      addr, 
                      __ksyms.symtable[best_index].addr, 
                      best_diff);
            twritef("%s\n", __ksyms.symtable[best_index].name);
        } else {
            //dbgprintf("Unknown symbol: 0x%lx\n", addr);
        }
}

void __backtrace_from(uintptr_t* frame_ptr, uintptr_t* return_addr)
{
    int depth = 0;

    while (frame_ptr && depth < KSYMS_MAX_DEPTH) {
        if(return_addr == 0) break;
        if(return_addr > 0x100000) break; // TODO: fix this (it's a hack to prevent backtracing into user space)

       __backtrace_find_symbol(return_addr);
        
        frame_ptr = (uintptr_t*) (*frame_ptr);
        if (frame_ptr) {
            return_addr = *(frame_ptr + 1);
        }
        depth++;
    }
}

void backtrace(void) {
    uintptr_t* frame_ptr = __builtin_frame_address(0);
    uintptr_t return_addr = *(frame_ptr + 1);
    __backtrace_from(frame_ptr, return_addr);
}
EXPORT_KSYMBOL(backtrace);

