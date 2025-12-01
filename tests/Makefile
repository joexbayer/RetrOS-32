include ../make/common.mk

BUILD_DIR := bin
INCLUDE_DIR := ../include

TEST_INCLUDES := -I $(INCLUDE_DIR) -I ./include/
COMMON_FLAGS := $(ARCH_CFLAGS) -O2 -Wall --no-builtin
MOCK := utils/mocks.c utils/test.c

FSBOJS = ../fs/bin/ext.o ../fs/bin/path.o ../fs/bin/inode.o
FATOBJS = ../fs/bin/fat16.o ../fs/bin/fat16_read.o ../fs/bin/fat16_write.o

TESTS := ext_test fat16_test pcb_test mem_test net_test http_test

.PHONY: all $(TESTS) run clean bin
all: $(TESTS) run

bin:
	$(call make_dir,$(BUILD_DIR))

ext_test: bin ext_test.c
	$(QUIET)$(CC) ext_test.c -D__FS_TEST ../bin/bitmap.o $(FSBOJS) -D__RetrOS32MOCK $(MOCK) $(TEST_INCLUDES) $(COMMON_FLAGS) -D__KERNEL -o $(BUILD_DIR)/ext_test.o

fat16_test: bin fat16_test.c
	$(QUIET)$(CC) fat16_test.c -D__FS_TEST ../bin/bitmap.o $(FATOBJS) -D__RetrOS32MOCK $(MOCK) $(TEST_INCLUDES) $(COMMON_FLAGS) -D__KERNEL -o $(BUILD_DIR)/fat16_test.o

mem_test: bin mem_test.c
	$(QUIET)$(CC) mem_test.c -D__MEM_TEST ../bin/bitmap.o ../bin/kmem.o -D__RetrOS32MOCK $(MOCK) $(TEST_INCLUDES) $(COMMON_FLAGS) -o $(BUILD_DIR)/mem_test.o

pcb_test: bin pcb_test.c
	$(QUIET)$(CC) pcb_test.c ../bin/bitmap.o ../bin/pcb_queue.o -D__RetrOS32MOCK $(MOCK) $(TEST_INCLUDES) $(COMMON_FLAGS) -o $(BUILD_DIR)/pcb_test.o

net_test: bin net_test.c
	$(QUIET)$(CC) net_test.c ../bin/rbuffer.o -D__RetrOS32MOCK $(MOCK) $(TEST_INCLUDES) $(COMMON_FLAGS) -D__KERNEL -o $(BUILD_DIR)/net_test.o

http_test: bin http_test.c
	$(QUIET)$(CC) http_test.c ../lib/http.c -D__RetrOS32MOCK $(MOCK) $(TEST_INCLUDES) $(COMMON_FLAGS) -o $(BUILD_DIR)/http_test.o

run:
	$(BUILD_DIR)/ext_test.o
	$(BUILD_DIR)/mem_test.o
	$(BUILD_DIR)/fat16_test.o
	$(BUILD_DIR)/pcb_test.o
	$(BUILD_DIR)/net_test.o
	$(BUILD_DIR)/http_test.o

clean:
	$(QUIET)$(RM_RF) $(BUILD_DIR)
	$(QUIET)$(RM_F) filesystem.test
