include ../make/common.mk

BUILD_DIR := bin
OUTPUTDIR := ../bin
INCLUDE_DIR := ../include

CFLAGS := $(FREESTANDING_CFLAGS) -std=gnu11 -O1 -D__KERNEL -I $(INCLUDE_DIR)
ASFLAGS := $(ARCH_ASFLAGS)
LDFLAGS := $(ARCH_LDFLAGS)

EXTOBJS := $(addprefix $(BUILD_DIR)/,path.o ext.o inode.o vfs.o)
FAT16OBJS := $(addprefix $(BUILD_DIR)/,fat16_read.o fat16_write.o fat16.o)
FSCORE := $(addprefix $(BUILD_DIR)/,fs.o fat16_iface.o)
FSOBJS := $(EXTOBJS) $(FAT16OBJS) $(FSCORE)
DEPS := $(FSOBJS:.o=.d)

.PHONY: all ext fat16 fs clean bindir
all: ext fat16 fs

ext: $(OUTPUTDIR)/ext.o
fat16: $(OUTPUTDIR)/fat16.o
fs: $(OUTPUTDIR)/fs.o

$(OUTPUTDIR)/ext.o: $(EXTOBJS) | bindir
	$(QUIET)$(LD) -o $@ $(addprefix ./,$(EXTOBJS)) $(LDFLAGS) -r
	@echo "[fs  ] linked $@"

$(OUTPUTDIR)/fat16.o: $(FAT16OBJS) | bindir
	$(QUIET)$(LD) -o $@ $(addprefix ./,$(FAT16OBJS)) $(LDFLAGS) -r
	@echo "[fs  ] linked $@"

$(OUTPUTDIR)/fs.o: $(FSCORE) | bindir
	$(QUIET)$(LD) -o $@ $(addprefix ./,$(FSCORE)) $(LDFLAGS) -r
	@echo "[fs  ] linked $@"

-include $(DEPS)

$(BUILD_DIR)/%.o: %.c | bindir
	$(QUIET)$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	@echo "[fs  ] cc $(<F)"

bindir:
	$(call make_dir,$(BUILD_DIR))

clean:
	$(QUIET)$(RM_F) $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d
	$(QUIET)$(RM_F) $(OUTPUTDIR)/ext.o $(OUTPUTDIR)/fat16.o $(OUTPUTDIR)/fs.o
