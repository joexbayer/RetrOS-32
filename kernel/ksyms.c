
#include <util.h>
#include <ksyms.h>
#include <serial.h>
#include <terminal.h>
#include <assert.h>

#define KSYMS_MAX_SYMBOLS 100
#define KSYMS_MAX_DEPTH 100

#define KSYMS_MAX_SYMBOL_LENGTH 25

#define EXPORT_KSYMBOL(func) \
    static void __register_##func() { add_symbol(#func, (uintptr_t)func); } \
    __attribute__((section(".symbol_table"))) const struct symbol_entry* __ptr_##func = &__register_##func;

static struct symbol_entry {
    char name[KSYMS_MAX_SYMBOL_LENGTH];
    uintptr_t addr;
} symtable[KSYMS_MAX_SYMBOLS];
static int num_symbols = 0;

void add_symbol(const char* name, uintptr_t addr) {
    
    assert(strlen(name) < KSYMS_MAX_SYMBOL_LENGTH);

    if (num_symbols < KSYMS_MAX_SYMBOLS) {
        memcpy(symtable[num_symbols].name, name, strlen(name));
        symtable[num_symbols].addr = addr;
        num_symbols++;
    } else {
        twrite("Error: symbol table full\n");
    }
    dbgprintf("Added new symbol %s\n", name);
}

void register_symbols()
{
    extern void  (*__start_symbol_table)();
    extern void  (*__start_symbol_table2)();
    extern void  (*__stop_symbol_table)();
}

EXPORT_KSYMBOL(add_symbol)

void backtrace(void) {
    uintptr_t* frame_ptr = __builtin_frame_address(0);;
    uintptr_t return_addr;
    int depth = 0;

    dbgprintf("Backtrace:\n");
    while (frame_ptr && depth < KSYMS_MAX_DEPTH) {
        return_addr = *((uintptr_t*) (frame_ptr + 1));
        
        int best_index = -1;
        int best_diff = 0x999999;
        for (int i = 0; i < num_symbols; i++) {
            if(symtable[i].addr-return_addr < best_diff){
                best_diff = symtable[i].addr-return_addr;
                best_index = i;
            }
        }
        if(best_index != -1)
            dbgprintf("%s: 0x%lx - 0x%lx = 0x%lx\n",symtable[best_index].name, return_addr, symtable[best_index].addr, symtable[best_index].addr-return_addr);
        
        frame_ptr = (uintptr_t*) (*frame_ptr);
        depth++;
    }
}

EXPORT_KSYMBOL(backtrace)