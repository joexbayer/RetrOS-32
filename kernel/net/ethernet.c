#include <net/ethernet.h>
#include <screen.h>

void print_ethernet(struct ethernet_header* hdr){
    scrprintf(1, 1, "dmac: %x:%x:%x:%x:%x:%x", hdr->dmac[0], hdr->dmac[1], hdr->dmac[2], hdr->dmac[3], hdr->dmac[4], hdr->dmac[5]);
    scrprintf(1, 2, "smac: %x:%x:%x:%x:%x:%x", hdr->smac[0], hdr->smac[1], hdr->smac[2], hdr->smac[3], hdr->smac[4], hdr->smac[5]);
    scrprintf(1, 3, "type: %x", hdr->ethertype);
}

void ethernet_parse();