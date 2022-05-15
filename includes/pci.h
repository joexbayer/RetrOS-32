#ifndef PCI_H
#define PCI_H

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint8_t pci_find_device(uint16_t find_vendor, uint16_t find_device);


#endif