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

/* From linux kernel. */
#define offsetof(st, m) \
    ((int)((char *)&((st *)0)->m - (char *)0))
#define container_of(ptr, type, member) ({         \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})


int strlen(const char* str);
uint32_t strncmp(const char* str, const char* str2, uint32_t len);

uint32_t memcmp(const void* ptr, const void* ptr2, uint32_t len);
void* memset (void *dest, int val, int len);
void* memcpy(void *dest, const void *src, int n);

#define CLI() asm ("cli")
#define STI() asm ("sti")

int atoi(char s[]);
void itoa(int n, char s[]); 
void itohex(uint32_t n, char s[]);

int isdigit(char c);
int rand(void);
#endif