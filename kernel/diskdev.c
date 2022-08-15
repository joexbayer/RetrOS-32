#include <diskdev.h>
#include <util.h>
#include <terminal.h>

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


int disk_size(){
    if(disk_device.attached)
        return disk_device.dev->size*512;

    return 0;
}

int write_block(char* buf, int block)
{
    dbgprintf("[DISK] write: 0x%x. Block %d\n", buf, block*512 - 122368);
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
    dbgprintf("[DISK] read: 0x%x. Block %d\n", buf, block*512 - 122368);
    return disk_device.read(buf, block, 1);
}

int read_block_offset(char* usr_buf, int size, int offset, int block)
{
    char buf[512];
    read_block((char*)buf, block);
    memcpy(usr_buf, &buf[offset], size);

    return size;   
}


void print_dev_status()
{
    
    if(disk_device.attached){
        static const char* SIZES[] = { "B", "kB", "MB", "GB" };
        uint32_t div_used = 0;
        uint32_t used = disk_device.dev->size*512;

        while (used >= 1024 && div_used < (sizeof SIZES / sizeof *SIZES)) {
            div_used++;   
            used /= 1024;
        }

        twritef("Disk Model: %s\n", disk_device.dev->model);
        twritef("Disk Size: %d%s, %d bytes, %d sectors.\n", used, SIZES[div_used], disk_device.dev->size*512, disk_device.dev->size);
        twriteln("Units: sectors of 1 x 512 = 512 bytes");
        twriteln("I/O Size: 512 bytes");

        return;
    }


    twriteln("No disk attached.");

}