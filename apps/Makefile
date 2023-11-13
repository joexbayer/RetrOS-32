CCFLAGS=-m32 -std=gnu11 -O2 \
		-g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
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

	CCFLAGS += -elf_i386
	ASFLAGS += --32
	LDFLAGS += -m elf_i386
else
	CC=i386-elf-gcc
	CPP=i386-elf-g++
	AS=i386-elf-as
	LD=i386-elf-ld
endif

OUTPUTDIR = ./bin/

UTILS = bin/cppUtils.o

USROBJS = bin/counter.o bin/clock.o bin/cube.o bin/echo.o

COMMON = ../bin/syscall.o ../bin/util.o ../bin/printf.o ../bin/graphics.o ../bin/netlib.o bin/cppUtils.o
LIBCOMMON_OBJS = ../bin/syscall.o ../bin/util.o ../bin/printf.o bin/cppUtils.o
LIBGRAPHICS_OBJS = ../bin/graphics.o
LIBNET_OBJS = ../bin/netlib.o


.PHONY: all new programs
all: new

new: ${UTILS} bindir programs

staticlibs:
	@ar rcs lib.a $(COMMON)

programs: staticlibs bin/crt0.o ${USROBJS}
	@make -C editor/
	@make -C colors/
	@make -C finder/
	@make -C calculator/
	@make -C graphs/
	@echo [USR] All user programs created and linked!

bin/%.o: %.c bin/crt0.o
	@$(CC) -o $@ -c $< $(CCFLAGS) 
	@$(LD) -o $(basename $@) $@ -L. -l:lib.a $(LDFLAGS) -T ./utils/linker.ld
	@cp $(basename $@) ../rootfs/bin/
	@echo [USR] Compiling $(basename $@)

bin/%.o: %.cpp bin/crt0.o
	@$(CPP) -o $@ -c $< $(CCFLAGS) -fno-exceptions -fno-rtti
	@$(LD) -o $(basename $@) $@ $(LDFLAGS) -L. -l:lib.a -T ./utils/linker.ld
	@cp $(basename $@) ../rootfs/bin/
	@echo [USR] Compiling $(basename $@)

bin/%.o: utils/%.cpp
	@$(CPP) -o $@ -c $< $(CCFLAGS) -fno-exceptions -fno-rtti -I ./../include/
	@echo [USR] Compiling C++ $(basename $@)

bin/%.o: */%.s
	@echo [USR] Compiling dependency $<
	@$(AS) -o $@ -c $< $(ASFLAGS)

.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@

-include .depend

bindir:
	@mkdir -p bin

clean:
	rm -f ./bin/*
	make -C editor/ clean
	make -C colors/ clean
	make -C finder/ clean
	make -C calculator/ clean
	rm -f .depend
