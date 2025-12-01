# Shared helpers and defaults used by RetrOS-32 Makefiles.

# Keep recursive make output compact.
MAKEFLAGS += --no-print-directory

# Toolchain detection.
UNAME_S := $(shell uname)
ifeq ($(UNAME_S),Darwin)
  CROSS_PREFIX ?= i386-elf-
else
  CROSS_PREFIX ?=
endif

CC  ?= $(CROSS_PREFIX)gcc
CXX ?= $(CROSS_PREFIX)g++
AS  ?= $(CROSS_PREFIX)as
LD  ?= $(CROSS_PREFIX)ld
AR  ?= $(CROSS_PREFIX)ar

# Architecture defaults.
ARCH_CFLAGS  ?= -m32
ARCH_ASFLAGS ?= --32
ARCH_LDFLAGS ?= -m elf_i386

# Freestanding flags shared by the kernel, drivers and apps that ship with the OS.
FREESTANDING_WARNINGS := -Wall -Wextra -Wpedantic -Wstrict-aliasing -Wno-pointer-arith -Wno-unused-parameter
FREESTANDING_FLAGS    := -nostdlib -nostdinc -ffreestanding -fno-pie -fno-stack-protector -fno-builtin-function -fno-builtin -fno-omit-frame-pointer
FREESTANDING_CFLAGS   := $(ARCH_CFLAGS) $(FREESTANDING_WARNINGS) $(FREESTANDING_FLAGS)
CPP_NO_RTTI           := -fno-exceptions -fno-rtti

# Dependency files for clang/gcc (-MMD).
DEPFLAGS ?= -MMD -MP

# Common utilities.
MKDIR_P ?= mkdir -p
RM_F    ?= rm -f
RM_RF   ?= rm -rf
QUIET   ?= @

# Helper for creating directories in recipes.
define make_dir
	$(QUIET)$(MKDIR_P) $(1)
endef
