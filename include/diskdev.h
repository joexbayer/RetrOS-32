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

int write_block(char* buf, int block);
int write_block_offset(char* usr_buf, int size, int offset, int block);

int read_block(char* buf, int block);
int read_block_offset(char* usr_buf, int size, int offset, int block);

#endif