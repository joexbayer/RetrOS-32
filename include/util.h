#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

/*
    Type Name	32–bit Size	    64–bit Size
    short	    2 bytes	        2 bytes
    int	        4 bytes	        4 bytes
    long	    4 bytes	        8 bytes
    long long	8 bytes	        8 bytes
*/

#define NULL (void *)0

extern long end;


size_t strlen(const char* str);
uint32_t strncmp(const char* str, const char* str2, uint32_t len);

uint32_t memcmp(const uint8_t* str, const uint8_t* str2, uint32_t len);
void* memset (void *dest, int val, size_t len);
void* memcpy(void *dest, const void *src, size_t n);

#define CLI() asm ("cli")
#define STI() asm ("sti")

int atoi(char s[]);
void itoa(int n, char s[]);
void itohex(uint32_t n, char s[]);

uint8_t inportb(uint16_t p);
uint16_t inportw(uint16_t p);
uint32_t inportl(uint16_t portid);

void outportb(uint16_t portid, uint8_t value);
void outportw(uint16_t portid, uint16_t value);
void outportl(uint16_t portid, uint32_t value);

int isdigit(char c);
int rand(void);
#endif