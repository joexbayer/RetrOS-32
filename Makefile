CCFLAGS=-m32 -std=gnu99 -O2 \
		-g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-builtin -I ./include/
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

PROGRAMOBJ = bin/counter.o bin/shell.o bin/networking.o bin/dhcpd.o

KERNELOBJ = bin/kernel_entry.o bin/kernel.o bin/terminal.o bin/pci.o \
			bin/util.o bin/interrupts.o bin/irs_entry.o bin/timer.o \
			bin/keyboard.o bin/screen.o bin/pcb.o bin/memory.o bin/e1000.o \
			bin/sync.o bin/process.o bin/ata.o bin/bitmap.o bin/rtc.o \
			bin/diskdev.o bin/scheduler.o bin/net.o bin/fs.o bin/windowmanager.o \
			bin/serial.o bin/io.o ${PROGRAMOBJ}
BOOTOBJ = bin/bootloader.o

.PHONY: all new image clean boot net kernel grub
all: iso

new: clean compile createbin grubiso

grub: createbin grubiso testgrub

testgrub:
	sudo qemu-system-i386 -cdrom  myos.iso

createbin:
	rm -f legacy/multiboot/boot/myos.bin
	cp ./bin/kernelout legacy/multiboot/boot/myos.bin

grubiso:
	grub-mkrescue -o myos.iso legacy/multiboot/

cleaniso: iso clean

ls:
	find . -name '*.c' | xargs wc -l
	find . -name '*.h' | xargs wc -l

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
	@echo [KERNEL] Compiling $@
	@$(CC) -o $@ -c $< $(CCFLAGS)

bin/%.o: */*/%.c
	@echo [PROGRAM] Compiling $@
	@$(CC) -o $@ -c $< $(CCFLAGS)

bin/%.o: */%.s
	@echo [KERNEL] Compiling $@
	@$(AS) -o $@ -c $< $(ASFLAGS)

bin/build: ./buildtools/build.c
	@gcc ./buildtools/build.c -o ./bin/build
	@echo [BUILD] Compiling $@

bin/net.o: ./net/*.c
	@echo [NETWORKING] Compiling the network stack
	@make -C ./net/

bin/fs.o: ./fs/*.c
	@echo [FILESYSTEM] Compiling the filesystem
	@make -C ./fs/

iso: bin/build compile
	@echo [BUILD] Building ISO file and attaching filesystem.
	@./bin/build

iso2: compile
	@dd if=/dev/zero of=boot.iso bs=512 count=961
	@dd if=./bin/bootblock of=boot.iso conv=notrunc bs=512 seek=0 count=1
	@dd if=./bin/kernelout of=boot.iso conv=notrunc bs=512 seek=1 count=960
	@echo Created boot.iso.

compile: bindir bootblock kernel

img: iso
	mv boot.iso boot.img

clean:
	make -C ./net clean
	make -C ./fs clean
	rm -f ./bin/*.o
	rm -f ./bin/bootblock
	rm -f ./bin/kernelout
	rm -f .depend

bindir:
	@mkdir -p bin

docker-rebuild:
	docker-compose build --no-cache

docker:
	sudo docker-compose up

boot: check
	sudo dd if=boot.iso of=/dev/disk2 bs=512 count=961 seek=0
	sync

check:
	diskutil list
	read eas
	sudo diskutil unmountDisk /dev/disk2

vdi: cleanvid iso
	qemu-img convert -f raw -O vdi boot.iso boot.vdi

cleanvid:
	rm -f *.vdi

net:
	sudo tcpdump -qns 0 -X -r dump.dat -vvv -e

qemu:
	sudo qemu-system-i386 -device e1000,netdev=net0 -serial stdio -netdev user,id=net0 -object filter-dump,id=net0,netdev=net0,file=dump.dat boot.iso

run: docker qemu