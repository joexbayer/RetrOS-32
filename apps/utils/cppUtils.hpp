#ifndef E5147D46_AE49_4745_B908_19DC6B1521DB
#define E5147D46_AE49_4745_B908_19DC6B1521DB

#include <stdint.h>

#ifdef __APPLE__
typedef unsigned long       size_t;
#else
typedef unsigned int       size_t;
#endif

void *operator new(size_t size);
 
void *operator new[](size_t size);
void operator delete(void *p);
 
void operator delete[](void *p);

#endif /* E5147D46_AE49_4745_B908_19DC6B1521DB */
