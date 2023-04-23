
#include <util.h>
#include <stdint.h>
#include <terminal.h>

#define MAX_SYMBOLS 100
#define MAX_DEPTH 100

struct symbol_entry {
    const char* name;
    uintptr_t addr;
};

static struct symbol_entry symtable[MAX_SYMBOLS];
static int num_symbols = 0;

void add_symbol(const char* name, uintptr_t addr) {
    if (num_symbols < MAX_SYMBOLS) {
        symtable[num_symbols].name = name;
        symtable[num_symbols].addr = addr;
        num_symbols++;
    } else {
        twrite("Error: symbol table full\n");
    }
}

void backtrace(void) {
    uintptr_t* frame_ptr = __builtin_frame_address(0);;
    uintptr_t return_addr;
    int depth = 0;

    twritef("Backtrace:\n");
    while (frame_ptr && depth < MAX_DEPTH) {
        return_addr = *((uintptr_t*) (frame_ptr + 1));
        twritef("%d: ", depth);
        
        int i;
        for (i = 0; i < num_symbols; i++) {
            if (return_addr >= symtable[i].addr && return_addr < symtable[i].addr + 0x1000) {
                twritef("%s + 0x%lx\n", symtable[i].name, return_addr - symtable[i].addr);
                break;
            }
        }
        if (i == num_symbols) {
            twritef("unknown function + 0x%lx\n", return_addr);
        }
        
        frame_ptr = (uintptr_t*) (*frame_ptr);
        depth++;
    }
}