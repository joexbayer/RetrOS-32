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

FSBOJS = ../fs/bin/fs.o ../fs/bin/path.o ../fs/bin/inode.o
MOCK = mocks.c

.PHONY: bin

all: fs_test

bin:
	@mkdir -p bin

fs_test: bin fs_test.c
	@$(CC) fs_test.c -D__FS_TEST ../bin/bitmap.o $(FSBOJS) -D__NETOS_MOCK $(MOCK) -I ../include/  -O2 -m32 -Wall -g --no-builtin -o ./bin/fs_test.o

clean:
	rm -f ./bin/*
	rm -f filesystem.test
