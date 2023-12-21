CCFLAGS=-m32 -std=gnu11 -O1 \
		-Wall -Wextra -Wpedantic -Wstrict-aliasing -D__KERNEL \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-omit-frame-pointer -fno-builtin -I ./../include/
ASFLAGS=
LDFLAGS=
MAKEFLAGS += --no-print-directory

ifneq ($(words $(MAKECMDGOALS)),1)
.DEFAULT_GOAL = all
%:
	@$(MAKE) $@ --no-print-directory -rRf $(firstword $(MAKEFILE_LIST))
else
ifndef ECHO
T := $(shell $(MAKE) $(MAKECMDGOALS) --no-print-directory \
    -nrRf $(firstword $(MAKEFILE_LIST)) \
    ECHO="COUNTTHIS" | grep -c "COUNTTHIS")

N := x
C = $(words $N)$(eval N := x $N)
ECHO = echo -ne "\r[$(words $N)/$T] (`expr $C '*' 100 / $T`%)"
endif

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

EXTOBJS = path.o ext.o inode.o vfs.o
FAT16OBJS = fat16_read.o fat16_write.o fat16.o

.PHONY: all new ext fat16 clean bindir fs
all: new

new: bindir ext fat16 fs

ext: ${EXTOBJS}
	@$(LD) -o ${OUTPUTDIR}ext.o $(addprefix ./bin/,$^) $(LDFLAGS) -r

fat16: ${FAT16OBJS}
	@$(LD) -o ${OUTPUTDIR}fat16.o $(addprefix ./bin/,$^) $(LDFLAGS) -r

fs: fs.o fat16_iface.o
	@$(LD) -o ${OUTPUTDIR}fs.o $(addprefix ./bin/,$^) $(LDFLAGS) -r

.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@

-include .depend

bindir:
	@mkdir -p bin

# For assembling and compiling all .c and .s files.
%.o: %.c
	@$(ECHO) [FS] Compiling bin/$@
	@$(CC) -o bin/$@ -c $< $(CCFLAGS)

clean:
	rm -f ./bin/*.o
	rm -f .depend
endif