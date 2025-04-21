CCFLAGS=-m32 -std=gnu11 -O2 \
		-Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-builtin -I ./../include/
ASFLAGS=
LDFLAGS=
MAKEFLAGS += --no-print-directory

UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
	CC=gcc
	CPP=g++
	AS=as
	LD=ld
	AR=ar

	CCFLAGS += -elf_i386
	ASFLAGS += --32
	LDFLAGS += -m elf_i386
else
	CC=i386-elf-gcc
	CPP=i386-elf-g++
	AS=i386-elf-as
	LD=i386-elf-ld
	AR=i386-elf-ar
endif

OUTPUTDIR = ./bin/

UTILS = bin/cppUtils.o bin/crt0.o

USROBJS =

COMMON = ../bin/syscall.o ../bin/libc.o ../bin/printf.o ../bin/graphics.o ../bin/netlib.o bin/cppUtils.o
LIB_COMMON_OBJS = ../bin/syscall.o ../bin/libc.o ../bin/printf.o bin/cppUtils.o
LIB_GRAPHICS_OBJS = ../bin/graphics.o ../bin/libc.o
LIB_NET_OBJS = ../bin/netlib.o
LIB_ZLIB_OBJS = ../bin/lz.o ../bin/libc.o

.PHONY: all new programs
all: new

new: bindir  ${UTILS} programs

staticlibs:
	$(AR) rcs libcore.a $(LIB_COMMON_OBJS)
	$(AR) rcs libnet.a $(LIB_NET_OBJS)
	$(AR) rcs libgraphic.a $(LIB_GRAPHICS_OBJS)
	$(AR) rcs libzlib.a $(LIB_ZLIB_OBJS)


programs: staticlibs bin/crt0.o ${USROBJS}
	@make -C editor/
	@make -C wolfstein/
	@make -C colors/
	@make -C finder/
	@make -C calculator/
	@make -C graphs/
	@make -C cube/
	@make -C users/
	@make -C texed/
	@make -C webserver/
	@make -f Makefile.retros -C C-Compiler/
	@echo [USR] All user programs created and linked!

bin/%.o: utils/%.cpp
	@$(CPP) -o $@ -c $< $(CCFLAGS) -fno-exceptions -fno-rtti -I ./../include/
	@echo [USR] Compiling C++ $(basename $@)

bin/%.o: utils/crt/%.s
	@echo [USR] Compiling dependency $<
	@$(AS) -o $@ -c $< $(ASFLAGS)

.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@

-include .depend

bindir:
	@mkdir -p bin

clean:
	rm -f ./bin/*
	@make -C editor/ clean
	@make -C wolfstein/ clean
	@make -C colors/ clean
	@make -C finder/ clean
	@make -C calculator/ clean
	@make -C graphs/ clean
	@make -C cube/ clean
	@make -C users/ clean
	@make -C texed/ clean
	@make -C webserver/ clean
	@make -f Makefile.retros -C C-Compiler/ clean
	@echo [USR] All user programs cleaned!
	rm -f .depend
