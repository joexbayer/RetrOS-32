#include <diskdev.h>
#include <util.h>
#include <terminal.h>
#include <serial.h>

struct diskdev disk_device;

void attach_disk_dev(
    int (*read)(char* buffer, uint32_t from, uint32_t size), 
    int (*write)(char* buffer, uint32_t from, uint32_t size),
    struct ide_device* dev
){
    disk_device.read = read;
    disk_device.write = write;

    disk_device.attached = 1;

    disk_device.dev = dev;
}

int disk_attached()
{
    return disk_device.attached;
}

char* disk_name()
{
    return disk_device.dev->model;
}


int disk_size(){
    if(disk_device.attached)
        return disk_device.dev->size*512;

    return 0;
}

int write_block(char* buf, int block)
{
    dbgprintf("[DISK] write: 0x%x. Block %d\n", buf, block*512);
    return disk_device.write(buf, block, 1);
}

int write_block_offset(char* usr_buf, int size, int offset, int block)
{
    char buf[512];
    disk_device.read((char*)buf, block, 1);
    memcpy(&buf[offset], usr_buf, size);

    return write_block(buf, block);
}

int read_block(char* buf, int block)
{
    dbgprintf("[DISK] read: 0x%x. Block %d\n", buf, block*512);
    return disk_device.read(buf, block, 1);
}

int read_block_offset(char* usr_buf, int size, int offset, int block)
{
    char buf[512];
    read_block((char*)buf, block);
    memcpy(usr_buf, &buf[offset], size);

    return size;   
}