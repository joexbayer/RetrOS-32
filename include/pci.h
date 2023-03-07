#ifndef PCI_H
#define PCI_H

#include <stdint.h>
/*
    
register	offset	bits 31-24		bits 23-16		bits 15-8		bits 7-0
	  00		00	Device ID						Vendor ID
	  01		04	Status							Command
	  02		08	Class code		Subclass		Prog IF			Revision ID
	  03		0C	BIST			Header type		Latency Timer	Cache Line Size
	  04		10	Base address #0 (BAR0)
	  05		14	Base address #1 (BAR1)
	  06		18	Base address #2 (BAR2)
	  07		1C	Base address #3 (BAR3)
	  08		20	Base address #4 (BAR4)
	  09		24	Base address #5 (BAR5)
	  0A		28	Cardbus CIS Pointer
	  0B		2C	Subsystem ID					Subsystem Vendor ID
	  0C		30	Expansion ROM base address
	  0D		34	Reserved						Capabilities Pointer
	  0E		38	Reserved
	  0F		3C	Max latency		Min Grant		Interrupt PIN	Interrupt Line
*/

struct pci_device {
    
    uint32_t bus;
    uint32_t slot;
    uint32_t function;
    uint16_t device;
    uint16_t vendor;

	uint8_t irq;

    uint16_t class;

	uint32_t base;
    struct pci_driver *driver;

};

/* Struct used to register a driver with its init function and vendor / device. */
struct pci_driver {
	uint16_t vendor, device;
    void (*attach)(struct pci_device* dev);
};


uint16_t pci_read_word(uint16_t bus, uint16_t slot, uint16_t func, uint16_t offset);
uint8_t pci_find_device(uint16_t find_vendor, uint16_t find_device);
void pci_enable_device_busmaster(uint16_t bus, uint16_t slot, uint16_t function);
void pci_init();

void list_pci_devices();


#endif