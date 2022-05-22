#ifndef UTILS_H
#define UTILS_H

/*
    Type Name	32–bit Size	    64–bit Size
    short	    2 bytes	        2 bytes
    int	        4 bytes	        4 bytes
    long	    4 bytes	        8 bytes
    long long	8 bytes	        8 bytes
*/

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;

#define NULL (void *)0


size_t strlen(const char* str);

void* memset (void *dest, int val, size_t len);

#define CLI() asm ("cli")
#define STI() asm ("sti")

int atoi(char s[]);
void itoa(int n, char s[]);
void itohex(uint32_t n, char s[]);

uint32_t ntohl(uint32_t data);
uint32_t htonl(uint32_t data);
uint16_t ntohs(uint16_t data);
uint16_t htons(uint16_t data);

uint8_t inportb(uint16_t p);
uint16_t inportw(uint16_t p);
uint32_t inportl(uint16_t portid);

void outportb(uint16_t portid, uint8_t value);
void outportw(uint16_t portid, uint16_t value);
void outportl(uint16_t portid, uint32_t value);

int rand(void);
#endif