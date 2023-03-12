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

USROBJS = bin/counter.o bin/clock.o bin/hello.o
COMMON = ../bin/syscall.o ../bin/util.o ../bin/printf.o ../bin/graphics.o

.PHONY: all new programs
all: new

new: bindir programs

programs: bin/crt0.o ${USROBJS} 
	@echo [USR] All user programs created and linked!

bin/%.o: %.c bin/crt0.o
	@$(CC) -o $@ -c $< $(CCFLAGS)
	@$(LD) -o $(basename $@) ./bin/crt0.o $@ $(COMMON) $(LDFLAGS) -T ./utils/linker.ld
	@echo [USR] Compiling $(basename $@)

bin/%.o: */%.s
	@echo [USR] Compiling dependency $<
	@$(AS) -o $@ -c $< $(ASFLAGS)

# C++ compiling
bin/%.o: %.c++ bin/crt0.o ./bin/crti.o ./bin/crtn.o
	@g++ -o $@ -c $< -ffreestanding -O1 -Wall -Wextra -fno-exceptions -fno-rtti -I ./../include/ -m32 -fno-pie -nostdlib
	@$(LD) -o $(basename $@) ./bin/crt0.o $@ $(COMMON) $(LDFLAGS) -T ./utils/linker.ld
	@echo [USR] Compiling C++ $(basename $@)


.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@

-include .depend

bindir:
	@mkdir -p bin

clean:
	rm -f ./bin/*
	rm -f .depend
