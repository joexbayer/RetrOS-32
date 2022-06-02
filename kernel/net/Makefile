CCFLAGS=-m32 -std=gnu99 -O2 \
		-g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-builtin -I ./../../includes/
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

NETOBJS = netdev.o

.PHONY: all new network clean
all: new

new: network

network: ${NETOBJS}
	@$(LD) -o ../../bin/net.o $(addprefix ./bin/,$^) $(LDFLAGS)
	@echo Network Object Created

.depend: *.[csh]
	@$(CC) $(CCOPTS) -MM -MG *.[cs] > $@

# For assembling and compiling all .c and .s files.
%.o: %.c
	@$(CC) -o bin/$@ -c $< $(CCFLAGS)
	@echo Compiling $@

clean:
	rm -f ./bin/*.o
	rm -f .depend

-include .depend