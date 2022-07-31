#ifndef __DISK_H
#define  __DISK_H

#include <ata.h>

extern struct diskdev disk_device;

void attach_disk_dev(
    int (*read)(char* buffer, uint32_t from, uint32_t size), 
    int (*write)(char* buffer, uint32_t from, uint32_t size),
    struct ide_device* dev
);

struct diskdev {
    int32_t (*read)(char* buffer, uint32_t from, uint32_t size);
    int32_t (*write)(char* buffer, uint32_t from, uint32_t size);

    struct ide_device* dev;
};

#endif