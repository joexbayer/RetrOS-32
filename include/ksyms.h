#ifndef __KSYMS_H
#define __KSYMS_H

#include <kutils.h>
#include <stdint.h>

void ksyms_add_symbol(const char* name, uintptr_t addr);
uintptr_t ksyms_resolve_symbol(const char* name);
void ksyms_list(void);
int ksyms_init(void);

/**
 * @brief EXPORT_KSYMBOL
 * Exports symbol to the kernels symbol table
 * making the function available for backtracing. 
 * @param func function to export
 */
#define EXPORT_KSYMBOL(func) \
        static void __ksymbol_##func(){ ksyms_add_symbol(#func, (uintptr_t)(func)); };\
        EXPORT_KCTOR(__ksymbol_##func)

#endif // !__KSYMS_H