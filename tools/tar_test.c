#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define the size of a TAR block */
#define TAR_BLOCK_SIZE 512

/* Define the structure of a TAR header block */
typedef struct tar_header {
    char name[100];       /* Name of the file */
    char mode[8];         /* File mode */
    char uid[8];          /* Owner's numeric user ID */
    char gid[8];          /* Group's numeric user ID */
    char size[12];        /* File size in bytes (octal base) */
    char mtime[12];       /* Last modification time in numeric Unix time format (octal) */
    char chksum[8];       /* Checksum for header block */
    char typeflag;        /* Type flag */
    char linkname[100];   /* Name of linked file */
    char magic[6];        /* USTAR indicator */
    char version[2];      /* USTAR version */
    char uname[32];       /* Owner user name */
    char gname[32];       /* Owner group name */
    char devmajor[8];     /* Device major number */
    char devminor[8];     /* Device minor number */
    char prefix[155];     /* Prefix for file names longer than 100 characters */
    char pad[12];         /* Padding to make the header up to 512 bytes */
} tar_header_t;

/* Function to convert octal string to unsigned int */
int octal_string_to_int(char *current_char, unsigned int size){
    unsigned int output = 0;
    while(size > 0){
        output = output * 8 + *current_char - '0';
        current_char++;
        size--;
    }
    return output;
}
/* Main function */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <tarfile>\n", argv[0]);
        return 1;
    }

    FILE *tar_file = fopen(argv[1], "rb");
    if (!tar_file) {
        perror("Error opening file");
        return 1;
    }

    tar_header_t header;
    size_t bytes_read;

    while ((bytes_read = fread(&header, 1, sizeof(header), tar_file)) > 0) {
        if (bytes_read < sizeof(header)) {
            printf("Incomplete header read.\n");
            break;
        }

        /* Check for two consecutive empty blocks indicating end of archive */
        if (header.name[0] == '\0') {
            printf("End of archive.\n");
            break;
        }

        printf("File: %s\n", header.name);
        printf("Mode: %.*s\n", (int)sizeof(header.mode), header.mode);
        printf("UID: %.*s\n", (int)sizeof(header.uid), header.uid);
        printf("GID: %.*s\n", (int)sizeof(header.gid), header.gid);
        printf("Size: %u bytes\n", octal_string_to_int(header.size, sizeof(header.size)));
        printf("Modification Time: %.*s\n", (int)sizeof(header.mtime), header.mtime);
        printf("Typeflag: %c\n", header.typeflag);
        printf("Linkname: %.*s\n", (int)sizeof(header.linkname), header.linkname);
        printf("Uname: %.*s\n", (int)sizeof(header.uname), header.uname);
        printf("Gname: %.*s\n", (int)sizeof(header.gname), header.gname);

        /* Calculate the size of the file */
        unsigned int size = octal_string_to_int(header.size, 11);

        /* Calculate the number of blocks to skip */
        unsigned int blocks_to_skip = (size + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE;

        /* Seek over the file's data blocks */
        fseek(tar_file, blocks_to_skip * TAR_BLOCK_SIZE, SEEK_CUR);
    }

    fclose(tar_file);
    return 0;
}
