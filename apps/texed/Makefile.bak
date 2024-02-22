### Standard Unix Environment Makefile ###
# This makefile is adapted to compile with ncurses
# in a standard Unix-like environment.
##############################################
CC=gcc
CPP=g++
AS=as
LD=ld
UNAME := $(shell uname)
ifeq ($(UNAME),Linux)
	CCFLAGS=-O2 -Wall -Wextra -Wpedantic -Iinclude/ -Iapps/
	LDFLAGS=-lncurses
else
	# For macOS, adjust as needed (e.g., clang, ncurses path)
	CCFLAGS=-O2  -Iinclude/ -I/usr/local/opt/ncurses/include -DNCURSES 
	LDFLAGS=-L/usr/local/opt/ncurses/lib -lncurses
endif

#### ONLY EDIT THIS ####
OUTPUT = texed
SRC_FILES = $(wildcard *.c)
OBJ_FILES = $(SRC_FILES:%.c=%.o)
########################

all: $(OUTPUT)

$(OUTPUT): $(OBJ_FILES)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f *.o $(OUTPUT)
