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

extern float sin_12[12];
extern float cos_12[12];
extern float cos_60[60];
extern float sin_60[60];

struct coordiante {
    int x;
    int y;
};

#ifdef __cplusplus
extern "C"
{
#endif

int strlen(const char* str);
uint32_t strncmp(const char* str, const char* str2, uint32_t len);

uint32_t memcmp(const void* ptr, const void* ptr2, uint32_t len);
void* memset (void *dest, int val, int len);
void* memcpy(void *dest, const void *src, int n);

int parse_arguments(const char *input_string, char *tokens[]);

int atoi(char s[]);
void itoa(int n, char s[]); 
void itohex(uint32_t n, char s[]);

int isdigit(char c);
int rand(void);

#ifdef __cplusplus
}
#endif


/* From linux kernel. */
#define offsetof(st, m) \
    ((int)((char *)&((st *)0)->m - (char *)0))
#define container_of(ptr, type, member) ({         \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

#define HLT() asm ("hlt")

#define PANIC()\
     asm ("cli");\
     while(1)\

#define GET_ESP() asm ("esp");

extern int cli_cnt;
#define CLI()\
    cli_cnt++;\
    asm ("cli");\

#define STI()\
    cli_cnt--;\
    if(cli_cnt == 0){\
        asm ("sti");\
    }\

#define CRITICAL_SECTION(code_block) \
    do { \
        CLI(); \
        code_block \
        STI(); \
    } while (0)

#define ASSERT_CRITICAL() assert(cli_cnt > 0)

unsigned long long rdtsc(void);
#endif