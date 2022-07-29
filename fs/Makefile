CCFLAGS=-m32 -std=gnu99 -O2 \
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

OUTPUTDIR = ../bin/

NETOBJS = fs.o

.PHONY: all new fs clean bindir
all: new

new: bindir fs

fs: ${NETOBJS}
	@$(LD) -o ${OUTPUTDIR}fs.o $(addprefix ./bin/,$^) $(LDFLAGS) -r
	@echo [FS] Filesystem Object Created

.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@

-include .depend

bindir:
	@mkdir -p bin

# For assembling and compiling all .c and .s files.
%.o: %.c
	@echo [FS] Compiling bin/$@
	@$(CC) -o bin/$@ -c $< $(CCFLAGS)

clean:
	rm -f ./bin/*.o
	rm -f .depend
