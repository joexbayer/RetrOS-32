#ifndef __KSYMS_H
#define __KSYMS_H

#include <stdint.h>

void add_symbol(const char* name, uintptr_t addr);
void backtrace(void);
void register_symbols();

#endif // !__KSYMS_H