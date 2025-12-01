include ../make/common.mk

BUILD_DIR := bin
OUTPUTDIR := ../bin
INCLUDE_DIR := ../include

CFLAGS := $(FREESTANDING_CFLAGS) -std=gnu99 -O1 -I $(INCLUDE_DIR)
ASFLAGS := $(ARCH_ASFLAGS)
LDFLAGS := $(ARCH_LDFLAGS)

NETOBJS := $(addprefix $(BUILD_DIR)/,netdev.o ethernet.o skb.o arp.o ipv4.o utils.o icmp.o udp.o socket.o dns.o routing.o tcp.o net.o api.o interface.o networkmanager.o firewall.o)
DEPS := $(NETOBJS:.o=.d)

.PHONY: all network clean bindir
all: network

network: $(OUTPUTDIR)/net.o

$(OUTPUTDIR)/net.o: $(NETOBJS) | bindir
	$(QUIET)$(LD) -o $@ $(addprefix ./,$(NETOBJS)) $(LDFLAGS) -r
	@echo "[net ] linked $@"

-include $(DEPS)

$(BUILD_DIR)/%.o: %.c | bindir
	$(QUIET)$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	@echo "[net ] cc $(<F)"

bindir:
	$(call make_dir,$(BUILD_DIR))

clean:
	$(QUIET)$(RM_F) $(BUILD_DIR)/*.o $(BUILD_DIR)/*.d
	$(QUIET)$(RM_F) $(OUTPUTDIR)/net.o
