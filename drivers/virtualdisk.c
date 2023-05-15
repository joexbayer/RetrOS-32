#include <util.h>
#include <ata.h>
#include <diskdev.h>
#include <stdint.h>

/**
 * @brief In memory virtual harddisk
 * Mostly for testing on real hardware.
 */

#define VIRTUAL_DISK_MEMORY_START 0x700000
#define VIRTUAL_DISK_MEMORY_END   0x800000

struct ide_device virtual_disk = {
    .size = 0x800000-0x700000,
    .model = "Virtual Disk 1.0"
};

int virtual_read(char* buffer, uint32_t from, uint32_t size)
{
    char* addr = VIRTUAL_DISK_MEMORY_START + (from*512);
    memcpy(buffer, addr, size*512);
    return size;
}

int virtual_write(char* buffer, uint32_t from, uint32_t size)
{
    char* addr = VIRTUAL_DISK_MEMORY_START + (from*512);
    memcpy(addr, buffer, size*512);
    return size;
}

void virtual_disk_attach()
{
    attach_disk_dev(&virtual_read, &virtual_write, &virtual_disk);
}


