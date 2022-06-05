CCFLAGS=-m32 -std=gnu99 -O2 \
		-g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-builtin -I ./includes/
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

PROGRAMOBJ = bin/counter.o bin/shell.o bin/networking.o 

KERNELOBJ = bin/kernel_entry.o bin/kernel.o bin/terminal.o bin/pci.o \
			bin/util.o bin/interrupts.o bin/irs_entry.o bin/timer.o \
			bin/keyboard.o bin/screen.o bin/pcb.o bin/memory.o bin/e1000.o \
			bin/sync.o bin/process.o bin/net.o ${PROGRAMOBJ}
BOOTOBJ = bin/bootloader.o

.PHONY: all new image clean boot net kernel
all: compile

new: clean iso

cleaniso: iso clean 

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
	@echo make deepclean - Deletes all .o, .iso and kernel.

bootblock: $(BOOTOBJ)
	@$(LD) $(LDFLAGS) -o bin/bootblock $^ -Ttext 0x7C00 --oformat=binary
	@echo [BOOT] Bootblock created.

kernel: $(KERNELOBJ)
	@$(LD) -o bin/kernelout $^ $(LDFLAGS) -T ./kernel/linker.ld
	@echo [KERNEL] Kernel created.

.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@
	@echo [KERNEL] Creating dependencies
	
-include .depend

# For assembling and compiling all .c and .s files.
bin/%.o: */%.c
	@$(CC) -o $@ -c $< $(CCFLAGS)
	@echo [KERNEL] Compiling $@

bin/%.o: */*/%.c
	@$(CC) -o $@ -c $< $(CCFLAGS)
	@echo [PROGRAM] Compiling $@

bin/%.o: */%.s
	@$(AS) -o $@ -c $< $(ASFLAGS)
	@echo [KERNEL] Compiling $@

bin/net.o: ./kernel/net/*.c
	@make -C ./kernel/net/

iso: compile
	@dd if=/dev/zero of=boot.iso bs=512 count=961
	@dd if=./bin/bootblock of=boot.iso conv=notrunc bs=512 seek=0 count=1
	@dd if=./bin/kernelout of=boot.iso conv=notrunc bs=512 seek=1 count=960
	@echo Created boot.iso.

compile: bindir bootblock kernel

img: iso
	mv boot.iso boot.img

clean:
	make -C kernel/net clean
	rm -f ./bin/*.o
	rm -f ./bin/bootblock
	rm -f ./bin/kernelout
	rm -f .depend

bindir:
	@mkdir -p bin

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