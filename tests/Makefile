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

FSBOJS = ../fs/bin/ext.o ../fs/bin/path.o ../fs/bin/inode.o
FATOBJS = ../fs/bin/fat16.o ../fs/bin/fat16_read.o ../fs/bin/fat16_write.o

MOCK = utils/mocks.c utils/test.c

.PHONY: bin

all: ext_test fat16_test pcb_test mem_test run

bin:
	@mkdir -p bin

ext_test: bin ext_test.c
	@$(CC) ext_test.c -D__FS_TEST ../bin/bitmap.o $(FSBOJS) -D__RetrOS32MOCK $(MOCK) -I ../include/ -I ./include/  -O2 -m32 -Wall --no-builtin -D__KERNEL -o ./bin/ext_test.o

fat16_test: bin fat16_test.c
	@$(CC) fat16_test.c -D__FS_TEST ../bin/bitmap.o $(FATOBJS) -D__RetrOS32MOCK $(MOCK) -I ../include/ -I ./include/  -O2 -m32 -Wall --no-builtin -D__KERNEL -o ./bin/fat16_test.o 

mem_test: bin mem_test.c
	@$(CC) mem_test.c -D__MEM_TEST ../bin/bitmap.o ../bin/kmem.o  -D__RetrOS32MOCK $(MOCK) -I ../include/ -I ./include/  -O2 -m32 -Wall --no-builtin -o ./bin/mem_test.o

pcb_test: bin pcb_test.c
	@$(CC) pcb_test.c ../bin/bitmap.o ../bin/pcb_queue.o  -D__RetrOS32MOCK $(MOCK) -I ../include/ -I ./include/  -O2 -m32 -Wall --no-builtin -o ./bin/pcb_test.o

fat16:
	make -C ../ compile && make fat16_test && ./bin/fat16_test.o

fat16ch:
	fsck.fat -v -n filesystem.test

run:
	./bin/ext_test.o
	./bin/mem_test.o
	./bin/fat16_test.o
	./bin/pcb_test.o

clean:
	rm -f ./bin/*
	rm -f filesystem.test
