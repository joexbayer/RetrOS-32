#ifndef STDINT_H
#define STDINT_H

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef long long           int64_t;

#ifdef __CROSS_COMPILING__
typedef long unsigned int        size_t;
#else
typedef unsigned int   size_t;
#endif


typedef unsigned int        uintptr_t;
typedef int                 intptr_t;

#endif // !STDINT_H