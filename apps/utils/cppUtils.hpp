#ifndef E5147D46_AE49_4745_B908_19DC6B1521DB
#define E5147D46_AE49_4745_B908_19DC6B1521DB

#include <stdint.h>

/*
 * Keep C++ allocation signatures aligned with the compiler's internal size
 */
typedef __SIZE_TYPE__ cpp_size_t;

void *operator new(cpp_size_t size);
void *operator new[](cpp_size_t size);
void operator delete(void *p);
void operator delete[](void *p);

void operator delete(void* p, cpp_size_t index);
void operator delete[](void* p, cpp_size_t index);

#endif /* E5147D46_AE49_4745_B908_19DC6B1521DB */
