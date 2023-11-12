CCFLAGS=-m32 -O2 \
		-g -Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-builtin -I ./../../include/ -I ./../include/ -fno-exceptions -fno-rtti
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
OUTPUT = edit.o
########################

SRC_FILES = $(wildcard *.cpp)
OBJ_FILES = $(SRC_FILES:%.cpp=$(OUTPUTDIR)%.o)

OUTPUTDIR = ./bin/
COMMON = ../../bin/lex.o

all: $(OUTPUT)

$(OUTPUT): $(OBJ_FILES)
	$(LD) -o $@ $(LDFLAGS) $^ -L../ -l:lib.a $(COMMON) -T ./../utils/linker.ld
	@cp $(OUTPUT) ../../rootfs/bin

$(OUTPUTDIR)%.o: %.cpp
	$(CPP) $(CCFLAGS) -c $< -o $@

clean:
	rm -rf $(OUTPUTDIR)* *.o *.d $(OUTPUT) .depend
