CCFLAGS=-m32 -std=gnu99 -O2 -g -Wall -Wextra -Wpedantic -Wstrict-aliasing -Wno-pointer-arith -Wno-unused-parameter -nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector -fno-builtin-function -fno-builtin
ASFLAGS=
LDFLAGS=


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

KERNELOBJ = entry.o kernel.o terminal.o pci.o util.o
BOOTOBJ = bootloader.o

.PHONY: all new image clean
all: new

new: clean image

bootblock: $(BOOTOBJ)
	$(LD) $(LDFLAGS) -o bootblock $^ -Ttext 0x7C00 --oformat=binary

kernel: $(KERNELOBJ)
	$(LD) -o kernel $^ $(LDFLAGS) -T linker.ld

image: bootblock kernel
	dd if=/dev/zero of=image.iso bs=512 count=961
	dd if=bootblock of=image.iso conv=notrunc bs=512 seek=0 count=1
	dd if=kernel of=image.iso conv=notrunc bs=512 seek=1 count=960

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