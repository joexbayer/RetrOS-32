#ifndef __KERNEL_UTILS_H
#define __KERNEL_UTILS_H

#include <sync.h>
#include <stdint.h>
#include <args.h>

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

/* get / set gs register */
#define get_gs() ({ unsigned int gs; __asm__ __volatile__ ("mov %%gs, %0" : "=r" (gs)); gs; })
#define set_gs(val) __asm__ __volatile__ ("mov %0, %%gs" : : "r" (val))

#endif /* X86_REGISTERS_H */

extern char _start_kctor_table[];
extern char _stop_kctor_table[];
extern char _kctor_table_size[];

typedef char byte_t;
typedef unsigned char ubyte_t;

#define MAX_UINT16_T 0xFFFF
#define MAX_UINT32_T 0xFFFFFFFF

/* This is a interupt handler function */
#define __int_handler //__attribute__((interrupt))
/* This is a async callback function */
#define __async_callback
#define __callback
/* This is a thread entry function */
#define __kthread_entry
/* This function does not return */
#define __noreturn //__attribute__((noreturn))
#define __deprecated

/**
 * @brief EXPORT_KCTOR
 * Adds a function pointer to the kctor table.
 * All kernel constructors are called at boot.
 * Expected function signature:
 * @param func void func(void);
 */
#define EXPORT_KCTOR(func) \
    uintptr_t __kctor_ptr_##func __attribute__((section(".kctor_table"), unused)) = (uintptr_t)&func

void init_kctors();

/* Define a series of macros to count the number of arguments */
#define VA_NUM_ARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)

/* Define the main macro that takes variable number of arguments */
#define ARGS(...) VA_NUM_ARGS(__VA_ARGS__), (char*[]){__VA_ARGS__}


#define PTR_SIZE sizeof(void*)
#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(size, boundary) ((size) & ~((boundary) - 1));

/* Macro to convert a lowercase character to uppercase */
#define TO_UPPER(c)  (((c) >= 'a' && (c) <= 'z') ? ((c) - 'a' + 'A') : (c))

#define jmp(addr) __asm__ __volatile__ ("jmp *%0" : : "m" (addr))
#define call(addr) __asm__ __volatile__ ("call *%0" : : "m" (addr))
#define ret() __asm__ __volatile__ ("ret")

typedef volatile int signal_value_t;

/* exeception to the exernal naming as its never supposed to be accessed without macro. */
extern int __cli_cnt;

#define ENTER_CRITICAL()\
    __cli_cnt++;\
    asm ("cli");\

#define LEAVE_CRITICAL()\
    __cli_cnt--;\
    if(__cli_cnt == 0){\
        asm ("sti");\
    }\


#define HLT() asm ("hlt")
#define PANIC()\
     asm ("cli");\
     while(1)\

/* validate flag */
#define SET_FlAG(flags, flag) (flags |= flag)
#define HAS_FLAG(flags, flag) (flags & flag)

#define CLEAR_FLAG(flags, flag) (flags &= ~flag)
#define roundup(x, n) (((x) + (n) - 1) / (n) * (n))
#define rounddown(x, n) ((x) / (n) * (n))

/* From linux kernel. */
#define offsetof(st, m) \
    ((int)((char *)&((st *)0)->m - (char *)0))
#define container_of(ptr, type, member) ({         \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

#define STRINGIFY(x) #x

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define UNUSED(x) (void)(x)

#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

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
    
#define ASSERT_CRITICAL() assert(__cli_cnt > 0)

typedef enum {
    false = 0,
    true = 1
} bool_t;

typedef int kref_t;

struct unit {
    char* unit;
    int size;
};
struct unit calculate_size_unit(int bytes);

struct kref {
    int refs;
    spinlock_t spinlock;
};
int kref_get(struct kref* ref);
int kref_put(struct kref* ref);
int kref_init(struct kref* ref);

int32_t csprintf(char *buffer, const char *fmt, va_list args);

int align_to_pointer_size(int size);
unsigned char*  encode_run_length(const unsigned char* data, int length, unsigned char* out, int* encodedLength);
unsigned char* decode_run_length(const unsigned char* encodedData, int encodedLength, unsigned char* out, int* decodedLength);
int exec_cmd(char* str);
void kernel_panic(const char* reason);
void reboot();


#endif /* __KERNEL_UTILS_H */
