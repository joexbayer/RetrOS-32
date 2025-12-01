# Shared rules for individual RetrOS-32 applications.

ROOT ?= $(abspath $(CURDIR)/../..)
include $(ROOT)/make/common.mk

BUILD_DIR ?= bin
APP_NAME ?= $(notdir $(CURDIR))
APP_OUTPUT ?= $(APP_NAME).o
APP_LINKER_SCRIPT ?= $(ROOT)/apps/utils/linker.ld
APP_LIBS ?= -lcore -lgraphic
APP_LDFLAGS ?=
APP_INCLUDES ?= -I $(ROOT)/include -I $(ROOT)/apps -I . -I $(CURDIR)/include
APP_CXXFLAGS ?= $(FREESTANDING_CFLAGS) $(CPP_NO_RTTI) -O2 $(APP_INCLUDES)
APP_CFLAGS ?= $(FREESTANDING_CFLAGS) -std=gnu11 -O2 $(APP_INCLUDES)
CRT0 ?= $(ROOT)/apps/bin/crt0.o

APP_CPPSRCS ?= $(wildcard *.cpp)
APP_CSRCS ?= $(wildcard *.c)
APP_OBJECTS := $(APP_CPPSRCS:%.cpp=$(BUILD_DIR)/%.o) $(APP_CSRCS:%.c=$(BUILD_DIR)/%.o)
APP_DEPS := $(APP_OBJECTS:.o=.d)
APP_LIBRARIES := $(patsubst -l%,$(ROOT)/apps/lib%.a,$(filter -l%,$(APP_LIBS)))

LDFLAGS += $(ARCH_LDFLAGS)

.PHONY: all install clean
all: $(APP_OUTPUT) install

install: $(APP_OUTPUT)
	$(call make_dir,$(ROOT)/rootfs/bin)
	$(QUIET)cp $(APP_OUTPUT) $(ROOT)/rootfs/bin
	$(QUIET)echo "Installed $(APP_OUTPUT) to $(ROOT)/rootfs/bin"

$(APP_OUTPUT): $(CRT0) $(APP_OBJECTS) $(APP_LIBRARIES)
	$(QUIET)$(LD) -o $@ $(LDFLAGS) $(APP_OBJECTS) -L$(ROOT)/apps $(APP_LIBS) $(APP_LDFLAGS) -T $(APP_LINKER_SCRIPT)

$(CRT0):
	$(MAKE) -C $(ROOT)/apps bin/crt0.o

$(APP_LIBRARIES):
	$(MAKE) -C $(ROOT)/apps staticlibs

$(BUILD_DIR)/%.o: %.cpp
	$(call make_dir,$(BUILD_DIR))
	$(QUIET)$(CXX) $(APP_CXXFLAGS) $(DEPFLAGS) -c $< -o $@
	$(QUIET)echo "[C++] $(<F)"

$(BUILD_DIR)/%.o: %.c
	$(call make_dir,$(BUILD_DIR))
	$(QUIET)$(CC) $(APP_CFLAGS) $(DEPFLAGS) -c $< -o $@
	$(QUIET)echo "[CC ] $(<F)"

-include $(APP_DEPS)

clean:
	$(QUIET)$(RM_RF) $(BUILD_DIR)
	$(QUIET)$(RM_F) $(APP_OUTPUT)
