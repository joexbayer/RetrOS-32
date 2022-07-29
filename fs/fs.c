#include <fs/fs.h>
#include <fs/superblock.h>
#include <diskdev.h>

int init_fs()
{
    /* Read superblock and check magic. */
    char buf[BLOCK_SIZE];

    disk_device.read(&buf, 100, 1);

    struct superblock* sb = buf;
}