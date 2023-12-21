CCFLAGS=-m32 -std=gnu99 -O1 \
		-Wall -Wextra -Wpedantic -Wstrict-aliasing \
		-Wno-pointer-arith -Wno-unused-parameter -nostdlib \
		-nostdinc -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin-function -fno-omit-frame-pointer -fno-builtin -I ./../include/
ASFLAGS=
LDFLAGS=
MAKEFLAGS += --no-print-directory

ifneq ($(words $(MAKECMDGOALS)),1)
.DEFAULT_GOAL = all
%:
	@$(MAKE) $@ --no-print-directory -rRf $(firstword $(MAKEFILE_LIST))
else
ifndef ECHO
T := $(shell $(MAKE) $(MAKECMDGOALS) --no-print-directory \
    -nrRf $(firstword $(MAKEFILE_LIST)) \
    ECHO="COUNTTHIS" | grep -c "COUNTTHIS")

N := x
C = $(words $N)$(eval N := x $N)
ECHO = echo -ne "\r[$(words $N)/$T] (`expr $C '*' 100 / $T`%)"
endif

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

OUTPUTDIR = ../bin/

NETOBJS = netdev.o ethernet.o skb.o arp.o ipv4.o utils.o icmp.o udp.o \
	socket.o dns.o routing.o tcp.o net.o api.o interface.o

.PHONY: all new network clean bindir
all: new

new: bindir network

network: ${NETOBJS}
	@$(LD) -o ${OUTPUTDIR}net.o $(addprefix ./bin/,$^) $(LDFLAGS) -r

.depend: **/*.[cSh]
	@$(CC) $(CCFLAGS) -MM -MG **/*.[cS] > $@

-include .depend

bindir:
	@mkdir -p bin

# For assembling and compiling all .c and .s files.
%.o: %.c
	@$(ECHO) [NET]        Compiling $<
	@$(CC) -o bin/$@ -c $< $(CCFLAGS)

clean:
	rm -f ./bin/*.o
	rm -f .depend
endif