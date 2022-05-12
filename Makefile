CCFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
ASFLAGS = 
LDFLAGS = 

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	GCC = gcc
	AS = as
	CCFLAGS += -march=i386 -m32
	ASFLAGS += -march=i386 -32
	LDFLAGS += -march=i386 -m32
else
	GCC = i686-elf-gcc
	AS = i686-elf-as
endif

all: clean link
	echo ${CCFLAGS}

%.o:%.c
	$(GCC) -c $(CCFLAGS) $< -o $@

%.o:%.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm *.o
	rm image

link: kernel.o boot.o
	$(GCC) -T linker.ld -o image -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc $(LDFLAGS)
run:
	qemu-system-i386 -kernel image