#include <diskdev.h>

struct diskdev disk_device;

void attach_disk_dev(
    int (*read)(char* buffer, uint32_t from, uint32_t size), 
    int (*write)(char* buffer, uint32_t from, uint32_t size)
){
    disk_device.read = read;
    disk_device.write = write;
}