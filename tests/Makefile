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

all: fs_test mem_test run

bin:
	@mkdir -p bin

fs_test: bin fs_test.c
	@$(CC) fs_test.c -D__FS_TEST ../bin/bitmap.o $(FSBOJS) -D__NETOS_MOCK $(MOCK) -I ../include/ -I ./include/  -O2 -m32 -Wall -g --no-builtin -o ./bin/fs_test.o

mem_test: bin mem_test.c
	@$(CC) mem_test.c -D__MEM_TEST ../bin/bitmap.o ../bin/kmem.o  -D__NETOS_MOCK $(MOCK) -I ../include/ -I ./include/  -O2 -m32 -Wall -g --no-builtin -o ./bin/mem_test.o

run:
	./bin/fs_test.o
	./bin/mem_test.o

clean:
	rm -f ./bin/*
	rm -f filesystem.test
