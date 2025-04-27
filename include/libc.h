#ifndef __LIBC_H
#define __LIBC_H

#include <stdint.h>
#include <args.h>
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

struct coordinate {
    int x;
    int y;
};

#ifdef __cplusplus
extern "C"
{
#endif

#define EXIT_FAILURE 1
int getopt(int argc, char* argv[], const char* optstring, char** optarg);

char* strtok(char* str, const char* delim);
char* strcat(char *dest, const char *src);
int strstr(const char* str, const char* str2);
char* strchr(const char* str, int ch);
int strlen(const char* str);
uint32_t strcpy(char* dest, const char* src);
uint32_t strcmp(const char* str, const char* str2);
uint32_t strncmp(const char* str, const char* str2, uint32_t len);
uint32_t strncpy(char* dest, const char* src, uint32_t len);

uint32_t memcmp(const void* ptr, const void* ptr2, uint32_t len);
void* memset (void *dest, int val, int len);
void* memcpy(void *dest, const void *src, int n);
void* xmemcpy(void *dest, const void *src, int n);
void* memmove(void *dest, const void *src, size_t n);

int32_t csprintf(char *buffer, const char *fmt, va_list args);

int parse_arguments(const char *input_string, char tokens[10][100]);

void hexdump(const void *data, int size);

int atoi(char s[]);
int itoa(int n, char s[]); 
int itohex(uint32_t n, char s[]);
int htoi(char s[]);

int isdigit(char c);
int isspace(char c);
int rand(void);

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

unsigned long long rdtsc(void);
#endif