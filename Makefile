include make/common.mk

ROOT_DIR := $(CURDIR)
BUILD_DIR := bin
INCLUDE_DIR := $(ROOT_DIR)/include
HOST_CC ?= gcc

CFLAGS := $(FREESTANDING_CFLAGS) -std=gnu11 -O1 -D__KERNEL -I $(INCLUDE_DIR)
ASFLAGS := $(ARCH_ASFLAGS)
LDFLAGS := $(ARCH_LDFLAGS)

HOSTFWD_PORT ?= 8080
QEMU_NETDEV = -netdev user,id=net0,hostfwd=tcp::$(HOSTFWD_PORT)-:80
QEMU_OPS = -device e1000,netdev=net0 -serial stdio $(QEMU_NETDEV) -object filter-dump,id=net0,netdev=net0,file=dump.dat -m 32m

ifeq ($(UNAME_S),Darwin)
  GRUB ?= grub-mkrescue /usr/local/lib/grub/i386-pc/ -o myos.iso legacy/multiboot
else
  GRUB ?= grub-mkrescue /usr/lib/grub/i386-pc/ -o myos.iso legacy/multiboot
endif

PROGRAMOBJ := $(addprefix $(BUILD_DIR)/,shell.o networking.o dhcpd.o tcpd.o logd.o taskbar.o about.o)
GFXOBJ := $(addprefix $(BUILD_DIR)/,window.o component.o composition.o gfxlib.o api.o theme.o core.o)
KERNELOBJ := $(addprefix $(BUILD_DIR)/,kernel.o terminal.o helpers.o pci.o virtualdisk.o windowmanager.o icons.o vga.o \
	libc.o interrupts.o irs_entry.o timer.o gdt.o smp.o keyboard.o pcb.o pcb_queue.o memory.o vmem.o kmem.o e1000.o display.o env.o conf.o \
	sync.o kthreads.o ata.o atapi.o bitmap.o rtc.o tss.o kutils.o login.o cmds.o diskdev.o scheduler.o work.o rbuffer.o errors.o kclock.o tar.o color.o loopback.o \
	serial.o io.o syscalls.o list.o hashmap.o vbe.o ksyms.o windowserver.o encoding.o mouse.o ipc.o sysinf.o font8.o net.o fs.o ext.o fat16.o partition.o \
	admin.o usermanager.o user.o group.o snake.o msgbox.o kevents.o textmode.o lz.o)
KERNELOBJ += $(PROGRAMOBJ) $(GFXOBJ)

BOOTOBJ := $(BUILD_DIR)/bootloader.o
LIBOBJ := $(addprefix $(BUILD_DIR)/,printf.o syscall.o graphics.o netlib.o http.o)
KERNEL_SUPPORT := $(addprefix $(BUILD_DIR)/,kcrt0.o multiboot.o)

ALL_OBJS := $(BOOTOBJ) $(KERNELOBJ) $(LIBOBJ) $(KERNEL_SUPPORT)
DEPS := $(ALL_OBJS:.o=.d)

.PHONY: all iso compile kernel multiboot_kernel bootblock symbols apps tools tests build img filesystem create_fs bare re_apps clean test bindir grub grub_fix qemu qemu-headless qemu_kernel docker docker-rebuild reset sync rsync mount run ls git vdi

all: img

iso: compile tests apps tools img
	@echo "Finished building ISO image."

compile: bindir $(LIBOBJ) bootblock kernel
	@echo "[compile] done."

bootblock: $(BOOTOBJ)
	$(QUIET)$(LD) $(LDFLAGS) -o $(BUILD_DIR)/bootblock $^ -Ttext 0x7C00 --oformat=binary

multiboot_kernel: $(BUILD_DIR)/multiboot.o $(KERNELOBJ)
	@echo "[kernel] linking multiboot kernel..."
	$(QUIET)$(LD) -o $(BUILD_DIR)/kernelout $^ $(LDFLAGS) -T ./boot/multiboot.ld

symbols: $(BUILD_DIR)/multiboot.o $(KERNELOBJ)
	@echo "[kernel] generating symbols..."
	$(QUIET)$(LD) -o $(BUILD_DIR)/symbols $^ $(LDFLAGS) -T ./kernel/linkersym.ld
	$(QUIET)nm -C -n $(BUILD_DIR)/symbols | grep ' [Tt] ' | sed 's/ [Tt] / /' > rootfs/sysutil/symbols.map

kernel: $(BUILD_DIR)/kcrt0.o $(KERNELOBJ)
	@echo "[kernel] linking kernel..."
	$(QUIET)$(LD) -o $(BUILD_DIR)/kernelout $^ $(LDFLAGS) -T ./kernel/linker.ld

build: bin/build

bin/build: tools/build.c $(BUILD_DIR)/fat16.o $(BUILD_DIR)/bitmap.o tests/utils/mocks.c
	$(call make_dir,$(BUILD_DIR))
	$(QUIET)$(HOST_CC) tools/build.c $(BUILD_DIR)/bitmap.o tests/utils/mocks.c $(BUILD_DIR)/fat16.o -I $(INCLUDE_DIR) -O2 -m32 -Wall -D__FS_TEST -D__KERNEL -no-pie -o $(BUILD_DIR)/build
	@echo "[host] built $(BUILD_DIR)/build"

tools: bin/build
	$(MAKE) -C tools

tests: compile
	$(MAKE) -C tests

$(BUILD_DIR)/net.o:
	$(MAKE) -C net

$(BUILD_DIR)/ext.o $(BUILD_DIR)/fat16.o $(BUILD_DIR)/fs.o:
	$(MAKE) -C fs

apps:
	$(MAKE) -C apps

img: tools compile apps symbols create_fs sync
	@echo "Finished creating the image."

filesystem:
	dd if=/dev/zero of=filesystem.image bs=512 count=390

create_fs: bin/build
	dd if=/dev/zero of=filesystem.image bs=512 count=390
	$(BUILD_DIR)/build

bare: compile create_fs

re_apps: apps create_fs sync

clean:
	$(MAKE) -C net clean
	$(MAKE) -C fs clean
	$(MAKE) -C apps clean
	$(MAKE) -C tests clean
	$(QUIET)$(RM_F) $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d $(BUILD_DIR)/bootblock $(BUILD_DIR)/kernelout $(BUILD_DIR)/symbols $(BUILD_DIR)/build
	$(QUIET)$(RM_F) .depend filesystem.image filesystem.test
	@echo "Cleaned kernel outputs."

test: clean compile tests

bindir:
	$(call make_dir,rootfs/bin)
	$(call make_dir,$(BUILD_DIR))

grub_fix:
	$(QUIET)$(RM_F) $(BUILD_DIR)/kernel.o

grub: CFLAGS += -DGRUB_MULTIBOOT
grub: grub_fix apps multiboot_kernel
	cp $(BUILD_DIR)/kernelout legacy/multiboot/boot/myos.bin
	$(GRUB)

qemu_kernel: CFLAGS += -DGRUB_MULTIBOOT
qemu_kernel: grub_fix multiboot_kernel
	qemu-system-i386 $(QEMU_OPS) -kernel $(BUILD_DIR)/kernelout

docker-rebuild:
	docker-compose build --no-cache

reset: clean img

docker:
	docker-compose up

vdi: clean docker
	qemu-img convert -f raw -O vdi boot.img boot.vdi

ifeq ($(OS),Windows_NT)
QEMU_CMD = qemu-system-i386.exe $(QEMU_OPS) -drive file=RetrOS-32-debug.img,format=raw,index=0,media=disk
else
QEMU_CMD = qemu-system-i386 $(QEMU_OPS) -drive file=RetrOS-32-debug.img,format=raw,index=0,media=disk
endif

QEMU_HEADLESS_CMD = $(QEMU_CMD) -display none

qemu:
	$(QEMU_CMD)

qemu-headless:
	$(QEMU_HEADLESS_CMD)

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
	@echo "Finished rsyncing."

mount:
	sudo mount -o shortname=winnt RetrOS-32-debug.img ./mnt

run: img qemu

ls:
	find -name '*.[ch]' | xargs wc -l

git:
	git submodule update --init --recursive

# Build rules
-include $(DEPS)

SRC_ROOTS := boot drivers fs graphics kernel lib apps net admin
SRC_DIRS  := $(foreach dir,$(SRC_ROOTS),$(shell find $(dir) -type d 2>/dev/null))
vpath %.c $(SRC_DIRS)
vpath %.s $(SRC_DIRS)

$(BUILD_DIR)/%.o: %.s
	$(call make_dir,$(BUILD_DIR))
	$(QUIET)$(AS) $(ASFLAGS) -c $< -o $@
	$(QUIET)echo "[AS ] $<"

# Prefer assembly sources when both .s and .c share a basename (e.g. helpers).
$(BUILD_DIR)/%.o: %.c
	$(call make_dir,$(BUILD_DIR))
	$(QUIET)$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	$(QUIET)echo "[CC ] $<"
