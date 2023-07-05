#ifndef E5147D46_AE49_4745_B908_19DC6B1521DB
#define E5147D46_AE49_4745_B908_19DC6B1521DB

#include <stdint.h>

void *operator new(long unsigned int size);
 
void *operator new[](long unsigned int size);
void operator delete(void *p);
 
void operator delete[](void *p);

#endif /* E5147D46_AE49_4745_B908_19DC6B1521DB */
