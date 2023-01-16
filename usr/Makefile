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
	AS=as
	LD=ld

	CCFLAGS += -elf_i386
	ASFLAGS += --32
	LDFLAGS += -m elf_i386
else
	CC=i386-elf-gcc
	AS=i386-elf-as
	LD=i386-elf-ld
endif

OUTPUTDIR = ./bin/

USROBJS = bin/counter.o
COMMON = ../bin/syscall.o ../bin/util.o ../bin/printf.o

.PHONY: all new programs
all: new

new: bindir programs

programs: bin/entry.o ${USROBJS} 
	@echo [USR] All user programs created and linked!

bin/%.o: %.c bin/entry.o
	@$(CC) -o $@ -c $< $(CCFLAGS)
	@$(LD) -o $(basename $@) $@ ./bin/entry.o $(COMMON) $(LDFLAGS) -T ./utils/linker.ld
	@echo [USR] Compiling $(basename $@)

bin/%.o: */%.s
	@echo [USR] Compiling dependency $@
	@$(AS) -o $@ -c $< $(ASFLAGS)

.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@

-include .depend

bindir:
	@mkdir -p bin

clean:
	rm -f ./bin/*
	rm -f .depend
