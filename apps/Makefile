include ../make/common.mk

ROOT_DIR := $(abspath ..)
BUILD_DIR := bin
INCLUDE_DIR := $(ROOT_DIR)/include

CFLAGS := $(FREESTANDING_CFLAGS) -std=gnu11 -O2 -I $(INCLUDE_DIR)
CXXFLAGS := $(FREESTANDING_CFLAGS) $(CPP_NO_RTTI) -O2 -I $(INCLUDE_DIR)
ASFLAGS := $(ARCH_ASFLAGS)
LDFLAGS += $(ARCH_LDFLAGS)

UTILS := $(addprefix $(BUILD_DIR)/,cppUtils.o crt0.o)
UTIL_DEPS := $(UTILS:.o=.d)

APPS := editor wolfstein colors finder calculator graphs cube users texed webserver

LIB_CORE_OBJS := ../bin/syscall.o ../bin/libc.o ../bin/printf.o $(BUILD_DIR)/cppUtils.o
LIB_GRAPHICS_OBJS := ../bin/graphics.o ../bin/libc.o
LIB_NET_OBJS := ../bin/netlib.o
LIB_ZLIB_OBJS := ../bin/lz.o ../bin/libc.o
LIB_HTTP_OBJS := ../bin/http.o ../bin/libc.o ../bin/printf.o ../bin/syscall.o

.PHONY: all programs staticlibs clean $(APPS) bindir
all: bindir $(UTILS) staticlibs programs

programs: $(UTILS)
	@for dir in $(APPS); do \
		$(MAKE) -C $$dir; \
	done
	$(MAKE) -f Makefile.retros -C C-Compiler
	@echo "[apps] built all user programs."

staticlibs: $(UTILS)
	$(QUIET)$(AR) rcs libcore.a $(LIB_CORE_OBJS)
	$(QUIET)$(AR) rcs libnet.a $(LIB_NET_OBJS)
	$(QUIET)$(AR) rcs libgraphic.a $(LIB_GRAPHICS_OBJS)
	$(QUIET)$(AR) rcs libzlib.a $(LIB_ZLIB_OBJS)
	$(QUIET)$(AR) rcs libhttp.a $(LIB_HTTP_OBJS)
	@echo "[apps] refreshed static libraries."

-include $(UTIL_DEPS)

$(BUILD_DIR)/%.o: utils/%.cpp | bindir
	$(QUIET)$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@
	$(QUIET)echo "[apps] c++ $(<F)"

$(BUILD_DIR)/%.o: utils/crt/%.s | bindir
	$(QUIET)$(AS) $(ASFLAGS) -c $< -o $@
	$(QUIET)echo "[apps] as $(<F)"

bindir:
	$(call make_dir,$(BUILD_DIR))

clean:
	@for dir in $(APPS); do \
		$(MAKE) -C $$dir clean; \
	done
	$(MAKE) -f Makefile.retros -C C-Compiler clean
	$(QUIET)$(RM_RF) $(BUILD_DIR)
	$(QUIET)$(RM_F) libcore.a libnet.a libgraphic.a libzlib.a libhttp.a
