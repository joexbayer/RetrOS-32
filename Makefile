CCFLAGS=-m32 -std=gnu11 -O1 -D__KERNEL \
		-Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-Wno-conversion -fno-omit-frame-pointer -I ./include/
ASFLAGS=
LDFLAGS= 
MAKEFLAGS += --no-print-directory

QEMU_OPS = -device e1000,netdev=net0 -serial stdio -netdev user,id=net0,hostfwd=tcp::80-:80 -object filter-dump,id=net0,netdev=net0,file=dump.dat -m 32m

# ---------------- For counting how many files to compile ----------------
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

# ---------------- For timing makefile ----------------
TIME_START := $(shell date +%s)
define TIME-END
@time_end=`date +%s` ; time_exec=`awk -v "TS=${TIME_START}" -v "TE=$$time_end" 'BEGIN{TD=TE-TS;printf "%02dd:%02dh:%02dm:%02ds\n",TD/(60*60*24),TD/(60*60)%24,TD/(60)%60,TD%60}'` ; echo "Build time: $${time_exec} for $@"
endef

# ---------------- For cross compilation (MacOS) ----------------
UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
	CC=gcc
	AS=as
	LD=ld

	CCFLAGS += -elf_i386
	ASFLAGS += --32
	LDFLAGS += -m elf_i386
	GRUB=grub-mkrescue /usr/lib/grub/i386-pc/ -o myos.iso legacy/multiboot
# ! Disable MacOS support for now, as it is not able to compile the build tools. !
else ifeq ($(UNAME),Darwin)
	GRUB=grub-mkrescue /usr/local/lib/grub/i386-pc/ -o myos.iso legacy/multiboot
	CC=i386-elf-gcc
	AS=i386-elf-as
	LD=i386-elf-ld
else
#	$(error This Makefile does not support building on this platform. Please use Linux, WSL or Docker)
endif

# ---------------- Objects to compile ----------------
PROGRAMOBJ = bin/shell.o bin/networking.o bin/dhcpd.o bin/tcpd.o bin/logd.o bin/taskbar.o bin/about.o

GFXOBJ = bin/window.o bin/component.o bin/composition.o bin/gfxlib.o bin/api.o bin/theme.o bin/core.o

KERNELOBJ = bin/kernel.o bin/terminal.o bin/helpers.o bin/pci.o bin/virtualdisk.o bin/windowmanager.o bin/icons.o bin/vga.o \
			bin/libc.o bin/interrupts.o bin/irs_entry.o bin/timer.o bin/gdt.o bin/smp.o \
			bin/keyboard.o bin/pcb.o bin/pcb_queue.o bin/memory.o bin/vmem.o bin/kmem.o bin/e1000.o bin/display.o bin/env.o bin/conf.o \
			bin/sync.o bin/kthreads.o bin/ata.o bin/atapi.o bin/bitmap.o bin/rtc.o bin/tss.o bin/kutils.o bin/login.o bin/cmds.o \
			bin/diskdev.o bin/scheduler.o bin/work.o bin/rbuffer.o bin/errors.o bin/kclock.o bin/tar.o bin/color.o bin/loopback.o \
			bin/serial.o bin/io.o bin/syscalls.o bin/list.o bin/hashmap.o bin/vbe.o bin/ksyms.o bin/windowserver.o bin/encoding.o\
			bin/mouse.o bin/ipc.o bin/sysinf.o ${PROGRAMOBJ} ${GFXOBJ} bin/font8.o bin/net.o bin/fs.o bin/ext.o bin/fat16.o bin/partition.o\
			bin/admin.o bin/usermanager.o bin/user.o bin/group.o bin/snake.o bin/msgbox.o bin/kevents.o bin/textmode.o bin/lz.o

BOOTOBJ = bin/bootloader.o

LIBOBJ = bin/printf.o bin/syscall.o bin/graphics.o bin/netlib.o

# ---------------- Makefile rules ----------------

.PHONY: all new image clean boot net kernel grub time tests build apps bin/build symbols git
all: iso
	$(TIME-END)

git:
	git submodule update --init --recursive

ls:
	find -name '*.[c|h]' | xargs wc -l

bootblock: $(BOOTOBJ)
	@$(LD) $(LDFLAGS) -o bin/bootblock $^ -Ttext 0x7C00 --oformat=binary

multiboot_kernel: bin/multiboot.o $(KERNELOBJ)
	@echo "[KERNEL]     Linking kernel..."
	@$(LD) -o bin/kernelout $^ $(LDFLAGS) -T ./boot/multiboot.ld
	@echo "[KERNEL]     Finished compiling kernel."

# Idea taken from SerenityOS
symbols: bin/multiboot.o $(KERNELOBJ)
	@echo "[KERNEL]     Creating symbols..."
	@$(LD) -o bin/symbols $^ $(LDFLAGS) -T ./kernel/linkersym.ld
	@nm -C -n bin/symbols | grep ' [Tt] ' | sed 's/ [Tt] / /' > rootfs/sysutil/symbols.map

kernel: bin/kcrt0.o $(KERNELOBJ)
	@echo "[KERNEL]     Linking kernel..."
	@$(LD) -o bin/kernelout $^ $(LDFLAGS) -T ./kernel/linker.ld

.depend: **/*.[cSh]	
	@echo [KERNEL] Creating dependencies...
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@
	
-include .depend

# For assembling and compiling all .c and .s files.
bin/%.o: */%.c
	@$(ECHO) [KERNEL]     Compiling $<
	@$(CC) -o $@ -c $< $(CCFLAGS)

bin/%.o: kernel/*/%.c
	@$(ECHO) [PROGRAM]    Compiling $<
	@$(CC) -o $@ -c $< $(CCFLAGS)

bin/%.o: */%.s
	@$(ECHO) [KERNEL]     Compiling $<
	@$(AS) -o $@ -c $< $(ASFLAGS)

bin/build: tools/build.c bin/fat16.o bin/bitmap.o ./tests/utils/mocks.c
	@gcc tools/build.c bin/bitmap.o ./tests/utils/mocks.c bin/fat16.o -I ./include/  -O2 -m32 -Wall -D__FS_TEST -D__KERNEL -o 	./bin/build
	@echo [BUILD]      Compiling $<

tools: bin/build

tests: compile
	@make -C ./tests/

bin/net.o: ./net/*.c
	@make -C ./net/

bin/ext.o: ./fs/*.c
	@make -C ./fs/

bin/fat16.o: ./fs/*.c
	@make -C ./fs/

apps:
	@make -C ./apps/

iso: compile tests apps tools build img
	$(TIME-END)

filesystem:
	@dd if=/dev/zero of=filesystem.image bs=512 count=390

compile: bindir $(LIBOBJ) bootblock kernel apps
	@echo "[Compile] Finished."
	$(TIME-END)

create_fs:
	@dd if=/dev/zero of=filesystem.image bs=512 count=390
	@./bin/build

bare: compile create_fs

img: tools compile symbols create_fs sync
	@echo "Finished creating the image."
	$(TIME-END)

re_apps: apps create_fs sync

clean:
	make -C ./net clean
	make -C ./fs clean
	make -C ./apps clean
	make -C ./tests clean
	rm -f ./bin/*.o
	rm -f ./bin/bootblock
	rm -f ./bin/kernelout
	rm -f .depend
	rm -f filesystem.image
	rm -f filesystem.test

test: clean compile tests

bindir:
	@mkdir -p rootfs/bin
	@mkdir -p bin

# Kernel.o must be recompiled with the multiboot flag.
grub_fix:
	@rm -f bin/kernel.o
grub: CCFLAGS += -DGRUB_MULTIBOOT
grub: grub_fix apps multiboot_kernel
	cp bin/kernelout legacy/multiboot/boot/myos.bin
	$(GRUB)

qemu_kernel: CCFLAGS += -DGRUB_MULTIBOOT
qemu_kernel: grub_fix grub_fix multiboot_kernel
	qemu-system-i386 $(QEMU_OPS) -kernel bin/kernelout

docker-rebuild:
	docker-compose build --no-cache

docker:
	docker-compose up

vdi: cleanvid docker
	qemu-img convert -f raw -O vdi boot.img boot.vdi

ifeq ($(OS),Windows_NT)
QEMU_CMD = qemu-system-i386.exe $(QEMU_OPS) -drive file=RetrOS-32-debug.img,format=raw,index=0,media=disk
else
QEMU_CMD = qemu-system-i386 $(QEMU_OPS) -drive file=RetrOS-32-debug.img,format=raw,index=0,media=disk
endif

qemu:
	$(QEMU_CMD)

sync:
	mkdir -p mnt
	sudo mount -o shortname=winnt RetrOS-32-debug.img ./mnt
	sudo cp -r ./rootfs/* ./mnt/
	sudo umount ./mnt
	@echo "Finished syncing."

rsync:
	mkdir -p mnt
	sudo mount -o shortname=winnt RetrOS-32-debug.img ./mnt
	sudo rsync -av --update ./rootfs/ ./mnt/
	sudo umount ./mnt
	@echo "Finisheds rsyncing."

mount:
	sudo mount -o shortname=winnt RetrOS-32-debug.img ./mnt

run: img qemu
endif
