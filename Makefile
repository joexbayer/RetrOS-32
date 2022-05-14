UNAME := $(shell uname)

ifeq ($(UNAME),Linux)
	CC=gcc -elf_i386
	AS=as --32
	LD=ld -m elf_i386
else
	CC=i386-elf-gcc
	AS=i386-elf-as
	LD=i386-elf-ld
endif

GFLAGS=
CCFLAGS=-m32 -std=gnu99 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing
CCFLAGS+=-Wno-pointer-arith -Wno-unused-parameter
CCFLAGS+=-nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector
CCFLAGS+=-fno-builtin-function -fno-builtin
ASFLAGS=
LDFLAGS=

all: new

new: clean image

bootsect: bootloader.o
	$(LD) -o bootblock $^ -Ttext 0x7C00 --oformat=binary

kernel: entry.o kernel.o terminal.o pci.o util.o
	$(LD) -o kernel $^ $(LDFLAGS) -T linker.ld

image: bootsect kernel
	dd if=/dev/zero of=boot.iso bs=512 count=2880
	dd if=bootblock of=image.iso conv=notrunc bs=512 seek=0 count=1
	dd if=kernel of=image.iso conv=notrunc bs=512 seek=1 count=2048

clean:
	rm -f *.o
	rm -f bootblock
	rm -f kernel
	rm -f *.iso
# For assembling and compiling all .c and .s files.
%.o: %.c
	$(CC) -o $@ -c $< $(CCFLAGS)

%.o: %.S
	$(AS) -o $@ -c $< $(ASFLAGS)