include ../make/common.mk

BUILD_DIR := bin
# build.c is compiled by the root Makefile into a host tool; skip it here.
SOURCES := $(filter-out build.c,$(wildcard *.c))
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean
all: $(OBJECTS)

$(BUILD_DIR)/%.o: %.c
	$(call make_dir,$(BUILD_DIR))
	$(QUIET)$(CC) -c $< -o $@
	@echo "[tool] cc $(<F)"

clean:
	$(QUIET)$(RM_RF) $(BUILD_DIR)
