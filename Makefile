CCFLAGS=-m32 -std=gnu99 -O2 \
		-g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-builtin -I ./includes/
ASFLAGS=
LDFLAGS=

version := $(shell date +%s )
versions := $(shell cat versions.txt)

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

PROGRAMOBJ = counter.o

KERNELOBJ = kernel_entry.o kernel.o terminal.o pci.o \
			util.o interrupts.o irs_entry.o timer.o \
			keyboard.o screen.o pcb.o memory.o e1000.o \
			shell.o sync.o process.o ${PROGRAMOBJ}
BOOTOBJ = bootloader.o

.PHONY: all new image clean boot
all: new

new: clean iso

help:
	@echo ~~~~~~~ Makefile commands ~~~~~~~ 
	@echo make - Creates a brand new .iso file.
	@echo make compile - Recompile kernel and bootloader.
	@echo make {bootblock / kernel} - Compiles the specified object.
	@echo 
	@echo ~~~~~~~ Ouput commands ~~~~~~~ 
	@echo make img - Create a .img file
	@echo make vdi - Create a .vdi file for virtualbox.
	@echo 
	@echo ~~~~~~~ Clean commands ~~~~~~~ 
	@echo make clean - Deletes all .o and .iso files.
	@echo make deepclean - Deletes all .o, .iso and kernel versions.
	@echo 
	@echo ~~~~~~~  Version commands ~~~~~~~ 
	@echo make versions - Lists all kernel versions since last deepclean.
	@echo make custom v={kernel} - Creates a .iso file with specified kernel.

bootblock: $(BOOTOBJ)
	$(LD) $(LDFLAGS) -o ./bin/bootblock ./bin/$^ -Ttext 0x7C00 --oformat=binary

kernel: $(KERNELOBJ)
	$(LD) -o ./bin/kernel-v${version} $(addprefix ./bin/,$^) $(LDFLAGS) -T ./kernel/linker.ld
	echo kernel-v${version}${tag} >> versions.txt

# For assembling and compiling all .c and .s files.
%.o: **/%.c
	$(CC) -o bin/$@ -c $< $(CCFLAGS)

%.o: */programs/%.c
	$(CC) -o bin/$@ -c $< $(CCFLAGS)

%.o: **/%.s
	$(AS) -o bin/$@ -c $< $(ASFLAGS)

iso: bindir bootblock kernel
	dd if=/dev/zero of=boot.iso bs=512 count=961
	dd if=./bin/bootblock of=boot.iso conv=notrunc bs=512 seek=0 count=1
	dd if=./bin/kernel-v${version} of=boot.iso conv=notrunc bs=512 seek=1 count=960

versions:
	@echo "Kernel versions since last deepclean: (Only local)"
	@echo ${versions}

# use this as make custom v={kernel-v*}
custom: bindir bootblock
	dd if=/dev/zero of=boot.iso bs=512 count=961
	dd if=./bin/bootblock of=boot.iso conv=notrunc bs=512 seek=0 count=1
	dd if=./bin/${v} of=boot.iso conv=notrunc bs=512 seek=1 count=960

compile: bootblock kernel

img: iso
	mv boot.iso boot.img

bindir:
	mkdir -p bin

clean: bindir
	rm -f ./bin/*.o
	rm -f ./bin/bootblock
	rm -f *.iso

deepclean: bindir
	rm -f ./bin/*
	rm -f *.iso
	rm -f versions.txt
	echo >versions.txt

boot: check
	sudo dd if=boot.iso of=/dev/disk2 bs=512 count=961 seek=0
	sync

check:
	diskutil list
	read eas
	sudo diskutil unmountDisk /dev/disk2

vdi: cleanvid
	qemu-img convert -f raw -O vdi boot.iso boot.vdi

cleanvid:
	rm *.vdi

net:
	sudo tcpdump -qns 0 -X -r dump.dat

run:
	qemu-system-i386 -device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::5555-:22 -object filter-dump,id=net0,netdev=net0,file=dump.dat boot.iso