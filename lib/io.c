#include <io.h>
/* IN and OUT for ports. */ 
uint8_t inportb(uint16_t p)
{
    uint8_t r;
    asm volatile("inb %%dx, %%al":"=a"(r):"d"(p));
    return r;
}

uint16_t inportw(uint16_t p)
{
    uint16_t r;
    asm volatile("inw %%dx, %%ax":"=a"(r):"d"(p));
    return r;
}

uint32_t inportl(uint16_t p)
{
    uint32_t r;
    asm volatile("inl %%dx, %%eax":"=a"(r):"d"(p));
    return r;
}

void outportb(uint16_t portid, uint8_t value)
{
    asm volatile("outb %%al, %%dx":: "d"(portid), "a"(value));
}

void outportw(uint16_t portid, uint16_t value)
{
    asm volatile("outw %%ax, %%dx":: "d"(portid), "a"(value));
}

void outportl(uint16_t portid, uint32_t value)
{
    asm volatile("outl %%eax, %%dx":: "d"(portid), "a"(value));
}