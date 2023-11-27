#include <lib/tar.h>
#include <terminal.h>
#include <fs/fs.h>
#include <ksyms.h>
#include <memory.h>

static int __octal_string_to_int(char *current_char, unsigned int size){
    unsigned int output = 0;
    while(size > 0){
        output = output * 8 + *current_char - '0';
        current_char++;
        size--;
    }
    return output;
}

static int tar_list(int fd)
{
    struct tar_header header;
    unsigned int bytes_read;

    while ((bytes_read = fs_read(fd, &header, sizeof(header))) > 0) {
        if (bytes_read < sizeof(header)) {
            twritef("Incomplete header read.\n");
            break;
        }

        /* Check for two consecutive empty blocks indicating end of archive */
        if (header.name[0] == '\0') {
            twritef("End of archive.\n");
            break;
        }
        
        /* Calculate the size of the file */
        unsigned int size = __octal_string_to_int(header.size, 11);
        /* Calculate the number of blocks to skip */
        unsigned int blocks_to_skip = (size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE;
        
        twritef("File: %s (%d bytes)\n", header.name, blocks_to_skip * TAR_BLOCK_SIZE);
        /* Seek over the file's data blocks */
        fs_seek(fd, blocks_to_skip * TAR_BLOCK_SIZE, FS_SEEK_CUR);
    }

    return 0;
}

static int tar_extract(int fd)
{
    struct tar_header header;
    unsigned int bytes_read;

    while ((bytes_read = fs_read(fd, &header, sizeof(header))) > 0) {
        
        if (bytes_read < sizeof(header)) {
            twritef("Incomplete header read.\n");
            break;
        }

        /* Check for two consecutive empty blocks indicating end of archive */
        if (header.name[0] == '\0') {
            twritef("End of archive.\n");
            break;
        }

        twritef("File: %s\n", header.name);

        /* Calculate the size of the file */
        unsigned int size = __octal_string_to_int(header.size, 11);
        /* Calculate the number of blocks to skip */
        unsigned int blocks_to_skip = (size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE;
        unsigned int bytes_to_read = TAR_BLOCK_SIZE * blocks_to_skip;

        byte_t* buf = (byte_t*)kalloc(bytes_to_read);
        if(buf == NULL){
            twritef("Error allocating memory\n");
            return -1;
        }

        int ret = fs_read(fd, buf, bytes_to_read);
        if(ret < 0){
            twritef("Error reading file\n");
            kfree(buf);
            return -1;
        }

        int new_file = fs_open(header.name, FS_FILE_FLAG_CREATE | FS_FILE_FLAG_WRITE);
        if(new_file < 0){
            twritef("Error creating file\n");
            kfree(buf);
            return -1;
        }

        fs_write(new_file, buf, bytes_to_read);

        fs_close(new_file);

        kfree(buf);
    }

    return 0;
}

/* these functions are for the kernel */
int tar(int argc, char* argv[])
{
    if (argc != 3) {
        twritef("Usage: %s [options] <tarfile>\n", argv[0]);
        twritef("Options:\n");
        twritef("  -c: create a tar file\n");
        twritef("  -x: extract a tar file\n");
        twritef("  -t: list the contents of a tar file\n");
        return 1;
    }

    char* opts = argv[1];
    if(opts[0] != '-'){
        twritef("Invalid option: %s \n", argv[1]);
        return 1;
    }

    int fd = fs_open(argv[2], FS_FILE_FLAG_READ);
    if(fd < 0){
        twritef("Error opening file\n");
        return 1;
    }

    switch (opts[1]) {
        case 'c':{
                twritef("ERROR: Currently unsupported!\n");
            }
            break;
        case 'x':{
                tar_extract(fd);
            }
            break;
        case 't':{
                tar_list(fd);
            }
            break;
        default:
            twritef("Invalid option: %s \n", argv[1]);
            return 1;
    }

    fs_close(fd);

    return 0;
}
EXPORT_KSYMBOL(tar);


