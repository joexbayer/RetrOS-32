#ifndef __KSYMS_H
#define __KSYMS_H

#include <stdint.h>

extern char _start_symbol_table[];
extern char _stop_symbol_table[];

extern char _symbol_table_size[];

void add_symbol(const char* name, uintptr_t addr);
void backtrace(void);
void register_symbols();

#endif // !__KSYMS_H