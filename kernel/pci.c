#include "terminal.h"
#include "util.h"
#include "pci.h"


uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;

    /* create configuration address */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
                (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

    /* write out the address */
    outl(0xCF8, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}


uint8_t pci_find_device(uint16_t find_vendor, uint16_t find_device)
{
    for(uint32_t bus = 0; bus < 256; bus++)
    {
        for(uint32_t slot = 0; slot < 32; slot++)
        {
            for(uint32_t function = 0; function < 8; function++)
            {
                    uint16_t vendor = pci_read_word(bus, slot, function, 0);
                    if(vendor == 0xffff) continue;
                   	if(vendor != find_vendor) continue;

                    uint16_t device = pci_read_word(bus, slot, function, 2);
					if(device == find_device)
					{
						return 1;
					}
            }
        }
    }
    return 0;
}