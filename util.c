#include "util.h"

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void outportl(uint16_t portid, uint32_t value)
{
        asm volatile("outl %%eax, %%dx": :"d" (portid), "a" (value));
}

uint32_t inportl(uint16_t portid)
{
        uint32_t ret;
        asm volatile("inl %%dx, %%eax":"=a"(ret):"d"(portid));
        return ret;
}