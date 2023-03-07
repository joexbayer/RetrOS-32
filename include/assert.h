#ifndef  __ASSERT_H
#define __ASSERT_H

#include <serial.h>
#include <util.h>

#define assert(expr) \
    if (!(expr)) { \
        dbgprintf("Assertion failed: %s, file %s, line %d\n", #expr, __FILE__, __LINE__); \
        PANIC();\
    }\

#define UNREACHABLE()\
        dbgprintf("Unreachable was reached: file %s, line %d\n", __FILE__, __LINE__); \
        PANIC();\

#endif // ! __ASSERT_H
