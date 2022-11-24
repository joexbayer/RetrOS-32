MAKEFLAGS += --no-print-directory

.PHONY: bin

all: fs_test

bin:
	@mkdir -p bin

fs_test: bin fs_test.c
	@gcc fs_test.c ../bin/bitmap.o ../fs/bin/fs.o ../fs/bin/inode.o -I ../include/  -O2 -m32 -Wall -pg -g --no-builtin -o ./bin/fs_test.o

clean:
	rm -f ./bin/*
	rm -f filesystem.test
