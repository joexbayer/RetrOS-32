#ifndef UTILS_H
#define UTILS_H

/*
    Type Name	32–bit Size	    64–bit Size
    short	    2 bytes	        2 bytes
    int	        4 bytes	        4 bytes
    long	    4 bytes	        8 bytes
    long long	8 bytes	        8 bytes
*/

#ifdef __INT8_TYPE__
typedef __INT8_TYPE__ int8_t;
#endif
#ifdef __INT16_TYPE__
typedef __INT16_TYPE__ int16_t;
#endif
#ifdef __INT32_TYPE__
typedef __INT32_TYPE__ int32_t;
#endif
#ifdef __INT64_TYPE__
typedef __INT64_TYPE__ int64_t;
#endif
#ifdef __UINT8_TYPE__
typedef __UINT8_TYPE__ uint8_t;
#endif
#ifdef __UINT16_TYPE__
typedef __UINT16_TYPE__ uint16_t;
#endif
#ifdef __UINT32_TYPE__
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT32_TYPE__ size_t;
#endif
#ifdef __UINT64_TYPE__
typedef __UINT64_TYPE__ uint64_t;
#endif


size_t strlen(const char* str);

void* memset (void *dest, int val, size_t len);

int atoi(char s[]);
void itoa(int n, char s[]);
void itohex(uint32_t n, char s[]);

uint32_t ntohl(uint32_t data);
uint32_t htonl(uint32_t data);
uint16_t ntohs(uint16_t data);
uint16_t htons(uint16_t data);

uint8_t inb(uint16_t p);
uint16_t inw(uint16_t p);
uint32_t inl(uint16_t portid);

void outb(uint16_t portid, uint8_t value);
void outw(uint16_t portid, uint16_t value);
void outl(uint16_t portid, uint32_t value);

#endif