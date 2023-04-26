#ifndef __KSYMS_H
#define __KSYMS_H

#include <kutils.h>
#include <stdint.h>

void ksyms_add_symbol(const char* name, uintptr_t addr);
void* ksyms_resolve_symbol(const char* name);
void backtrace(void);

/**
 * @brief EXPORT_KSYMBOL
 * Exports symbol to the kernels symbol table
 * making the function available for backtracing. 
 * @param func function to export
 */
#define EXPORT_KSYMBOL(func) \
    static void __register_##func() { ksyms_add_symbol(#func, (uintptr_t)func); } \
    EXPORT_KCTOR(__register_##func)

#endif // !__KSYMS_H