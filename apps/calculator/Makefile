CCFLAGS=-m32 -O2 \
		-g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-builtin -I ./../../include/ -I ./../include/
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
OUTPUT = calc.o
DEPENDS = bin/calc.o
########################

OUTPUTDIR = ./bin/
COMMON = ../../bin/syscall.o ../../bin/util.o ../../bin/graphics.o ../bin/cppUtils.o

.PHONY: all new programs
all: new bin $(OUTPUT)

new: bindir

# C++ compiling
bin/%.o: %.cpp
	$(CPP) -o $@ -c $< $(CCFLAGS) -fno-exceptions -fno-rtti -I ./../include/
	@echo [USR] Compiling C++ $(basename $@)

$(OUTPUT): $(DEPENDS)
	@$(LD) -o $@ $(DEPENDS) $(COMMON) $(LDFLAGS) -T ./../utils/linker.ld
	@echo [USR] Compiling C++ $(basename $@)
	@cp $(OUTPUT) ../../rootfs/bin

.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@

-include .depend

bindir:
	@mkdir -p bin

clean:
	rm -f ./bin/*
	rm -f $(OUTPUT)
	rm -f .depend
