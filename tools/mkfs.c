#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <fs/fs.h>
#include <fs/inode.h>
#include <fs/superblock.h>

static FILE* filesystem = NULL;

bitmap_t create_bitmap(int n)
{
    return (bitmap_t) malloc((n + 7) / 8);
}


int read_block(char* buf, int block)
{
    fseek(filesystem, block*512, SEEK_SET);
    
}

int main(int argc, char* argv[])
{
    /* Make a filesystem image with given binary programs  */
    filesystem = fopen("../filesystem.image", "rw");

    return 0;
}