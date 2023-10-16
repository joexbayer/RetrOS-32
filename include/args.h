#ifndef __ARGS_H
#define __ARGS_H

#ifndef __RetrOS32MOCK
#define va_start(v,l) __builtin_va_start(v,l)
#define va_arg(v,l)   __builtin_va_arg(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_copy(d,s)  __builtin_va_copy(d,s)
#endif

typedef __builtin_va_list va_list;

#endif // !__ARGS_H
