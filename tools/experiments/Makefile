CXX ?= g++
CC ?= gcc
CXXFLAGS := -std=c++11 -I../../apps/utils -I../../include -Wno-builtin-declaration-mismatch
CFLAGS := -I../../include
CLIB_SRCS := ../../lib/libc.c ../../lib/printf.c ../../lib/syscall.c
CLIB_OBJS := $(CLIB_SRCS:.c=.o)

.PHONY: http clean

http: http.cpp $(CLIB_OBJS)
	$(CXX) $(CXXFLAGS) $< $(CLIB_OBJS) -o $@
	@./$@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f http a.out $(CLIB_OBJS)
