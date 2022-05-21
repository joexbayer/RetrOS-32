#include <pci.h>
#include <screen.h>

static const char* pci_classes[] =
{
	[0x0] = "Unknown",
	[0x1] = "Storage controller",
	[0x2] = "Network controller",
	[0x3] = "Display controller",
	[0x4] = "Multimedia device",
	[0x5] = "Memory controller",
	[0x6] = "Bridge device",
};

pci_device_t _pci_devices[8];
int _pci_devices_size = 0;

/* https://wiki.osdev.org/PCI */
uint16_t pci_read_word(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset)
{
	uint32_t address;
    uint32_t lbus = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
    outportl (0xCF8, address);
    tmp = (uint16_t)((inportl (0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}

inline uint16_t pci_get_device_class(bus, slot, function)
{
    return (pci_read_word(bus,slot,function,0xA) & 0xFF00) >> 8;
}

void pci_register_device(uint32_t bus, uint32_t slot, uint32_t function, uint16_t vendor, uint16_t device, uint16_t class)
{

	pci_device_t pci_dev;

    pci_dev.device = device;
    pci_dev.vendor = vendor;
    pci_dev.bus = bus;
    pci_dev.slot = slot;
    pci_dev.function = function;
	pci_dev.class = class;

	_pci_devices[_pci_devices_size] = pci_dev;
	_pci_devices_size++;

}

void init_pci()
{
    int devices_found = 0;
    int pcb_x = 30;
    int pcb_y = 2;
    scrwrite(pcb_x, pcb_y-1, "PCI Devices: ", VGA_COLOR_LIGHT_GREEN);

    for(uint32_t bus = 0; bus < 256; bus++)
    {
        for(uint32_t slot = 0; slot < 32; slot++)
        {
            for(uint32_t function = 0; function < 8; function++)
            {
                    uint16_t vendor = pci_read_word(bus, slot, function, 0);
                    if(vendor == 0xffff) continue;

                    uint16_t device = pci_read_word(bus, slot, function, 2);
					uint16_t class = pci_get_device_class(bus, slot, function);

					pci_register_device(bus, slot, function, vendor, device, class);
					scrprintf(pcb_x, pcb_y+devices_found, "V: 0x%x, D: 0x%x, %s", vendor, device, pci_classes[class]);
                    devices_found++;
            }
        }
    }
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