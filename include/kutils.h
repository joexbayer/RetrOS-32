#ifndef __KERNEL_UTILS_H
#define __KERNEL_UTILS_H

#ifndef X86_REGISTERS_H
#define X86_REGISTERS_H

#define ESP ({ unsigned int esp; __asm__ __volatile__ ("mov %%esp, %0" : "=r" (esp)); esp; })
#define EIP ({ unsigned int eip; __asm__ __volatile__ ("call 1f\n1: pop %0" : "=r" (eip)); eip; })
#define EAX ({ unsigned int eax; __asm__ __volatile__ ("mov %%eax, %0" : "=r" (eax)); eax; })
#define EBX ({ unsigned int ebx; __asm__ __volatile__ ("mov %%ebx, %0" : "=r" (ebx)); ebx; })
#define ECX ({ unsigned int ecx; __asm__ __volatile__ ("mov %%ecx, %0" : "=r" (ecx)); ecx; })
#define EDX ({ unsigned int edx; __asm__ __volatile__ ("mov %%edx, %0" : "=r" (edx)); edx; })

#define lcr0(val) __asm__ __volatile__ ("mov %0, %%cr0" : : "r" (val))
#define lcr3(val) __asm__ __volatile__ ("mov %0, %%cr3" : : "r" (val))
#define lcr4(val) __asm__ __volatile__ ("mov %0, %%cr4" : : "r" (val))

#endif /* X86_REGISTERS_H */

extern char _start_kctor_table[];
extern char _stop_kctor_table[];
extern char _kctor_table_size[];


/* This is a interupt handler function */
#define __int_handler //__attribute__((interrupt))
/* This is a async callback function */
#define __async_callback
/* This is a thread entry function */
#define __kthread_entry
/* This function does not return */
#define __noreturn //__attribute__((noreturn))

/**
 * @brief EXPORT_KCTOR
 * Adds a function pointer to the kctor table.
 * All kernel constructors are called at boot.
 * Expected function signature:
 * @param func void func(void);
 */
#define EXPORT_KCTOR(func) uintptr_t __kctor_ptr_##func __attribute__((section(".kctor_table"), unused)) = (uintptr_t)&func

void init_kctors();


#define jmp(addr) __asm__ __volatile__ ("jmp *%0" : : "m" (addr))
#define call(addr) __asm__ __volatile__ ("call *%0" : : "m" (addr))
#define ret() __asm__ __volatile__ ("ret")

extern int cli_cnt;
#define ENTER_CRITICAL()\
    cli_cnt++;\
    asm ("cli");\

#define LEAVE_CRITICAL()\
    cli_cnt--;\
    if(cli_cnt == 0){\
        asm ("sti");\
    }\


#define HLT() asm ("hlt")
#define PANIC()\
     asm ("cli");\
     while(1)\

/**
 * @brief CRITICAL_SECTION
 * Enters critical section before the
 * code block is run and leaves after.
 * @param code_block block of code to run in critical section
 */
#define CRITICAL_SECTION(code_block) \
    do { \
        ENTER_CRITICAL(); \
        code_block \
        LEAVE_CRITICAL(); \
    } while (0)
    
#define ASSERT_CRITICAL() assert(cli_cnt > 0)

struct unit {
    char* unit;
    int size;
};
struct unit calculate_size_unit(int bytes);

unsigned char* run_length_encode(const unsigned char* data, int length, unsigned char* out, int* encodedLength);
unsigned char* run_length_decode(const unsigned char* encodedData, int encodedLength, unsigned char* out, int* decodedLength);
int exec_cmd(char* str);
void kernel_panic(const char* reason);



#endif /* __KERNEL_UTILS_H */
