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

int getopt(int argc, char* argv[], const char* optstring, char** optarg);
char* strtok(char* str, const char* delim);

char* strchr(const char* str, int ch);
int strlen(const char* str);

uint32_t strcmp(const char* str, const char* str2);
uint32_t strncmp(const char* str, const char* str2, uint32_t len);

uint32_t memcmp(const void* ptr, const void* ptr2, uint32_t len);
void* memset (void *dest, int val, int len);
void* memcpy(void *dest, const void *src, int n);

int parse_arguments(const char *input_string, char tokens[10][100]);

void hexdump(const void *data, int size);

int atoi(char s[]);
int itoa(int n, char s[]); 
int itohex(uint32_t n, char s[]);
int htoi(char s[]);

int isdigit(char c);
int isspace(char c);
int rand(void);

typedef volatile int signal_value_t;

struct args {
    int argc;
    char* argv[10];
    char data[10][100];
};

#define SAVE_AND_RESTORE(x, code_block) \
do { \
    decltype(x) _temp = x; \
    code_block \
    x = _temp; \
} while(0)


#ifdef __cplusplus
}
#endif


#define roundup(x, n) (((x) + (n) - 1) / (n) * (n))
#define rounddown(x, n) ((x) / (n) * (n))


/* From linux kernel. */
#define offsetof(st, m) \
    ((int)((char *)&((st *)0)->m - (char *)0))
#define container_of(ptr, type, member) ({         \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

#define STRINGIFY(x) #x

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define UNUSED(x) (void)(x)

#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

unsigned long long rdtsc(void);
#endif