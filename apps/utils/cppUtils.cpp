#include <stdint.h>
#include "cppUtils.hpp"
#include <lib/syscall.h>

void *operator new(unsigned int size)
{
    return malloc(size);
}
 
void *operator new[](unsigned int size)
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