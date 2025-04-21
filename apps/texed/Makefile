### RetrOS-32 v0.0.1 Makefile ###
# This makefile only works in the 
# RetrOS-32 Environment. It depends
# on static libraries.
##################################
ROOT = ../../

CCFLAGS=-m32 -O2 -Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-builtin -I $(ROOT)include/ -I $(ROOT)apps/
MAKEFLAGS += --no-print-directory
UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
	CC=gcc
	CPP=g++
	AS=as
	LD=ld

	CCFLAGS += -elf_i386
	ASFLAGS += --32
	LDFLAGS += -m elf_i386
else
	CC=i386-elf-gcc
	CPP=i386-elf-g++
	AS=i386-elf-as
	LD=i386-elf-ld
endif

#### ONLY EDIT THIS ####
OUTPUT = texed.o
LIBS = -lcore
EXTENDED = 
########################

SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:%.c=$(OUTPUTDIR)%.o)
OUTPUTDIR = ./bin/

all: $(OUTPUT) install

install: $(OUTPUT)
	@cp $(OUTPUT) $(ROOT)rootfs/bin
	@echo "Installed $(OUTPUT) to $(ROOT)rootfs/bin"

$(OUTPUT): $(OBJ_FILES)
	$(LD) -o $@ $(LDFLAGS) $^ -L$(ROOT)apps $(LIBS) $(EXTENDED) -T $(ROOT)apps/utils/linker.ld

$(OUTPUTDIR)%.o: %.c
	@mkdir -p $(OUTPUTDIR)
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -rf $(OUTPUTDIR)* *.o *.d $(OUTPUT) .depend
