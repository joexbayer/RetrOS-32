#include "util.h"

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

/*
	TODO:
	memcpy
	memset
	
*/

/* IN and OUT for ports. */ 
uint8_t inb(uint16_t p)
{
    uint8_t r;
    asm volatile("inb %%dx, %%al":"=a"(r):"d"(p));
    return r;
}

uint16_t inw(uint16_t p)
{
    uint16_t r;
    asm volatile("inw %%dx, %%ax":"=a"(r):"d"(p));
    return r;
}

uint32_t inl(uint16_t p)
{
    uint32_t r;
    asm volatile("inl %%dx, %%eax":"=a"(r):"d"(p));
    return r;
}

void outb(uint16_t portid, uint8_t value)
{
    asm volatile("outb %%al, %%dx":: "d"(portid), "a"(value));
}

void outw(uint16_t portid, uint16_t value)
{
    asm volatile("outw %%ax, %%dx":: "d"(portid), "a"(value));
}

void outl(uint16_t portid, uint32_t value)
{
    asm volatile("outl %%eax, %%dx":: "d"(portid), "a"(value));
}