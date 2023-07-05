#include <stdint.h>
#include "cppUtils.hpp"
#include <lib/syscall.h>

void *operator new(long unsigned int size)
{
    return malloc(size);
}
 
void *operator new[](long unsigned int size)
{
    return malloc(size);
}
 
void operator delete(void *p)
{
    free(p);
}
 
void operator delete[](void *p)
{
    free(p);
}