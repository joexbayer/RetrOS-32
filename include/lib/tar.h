#ifndef __TAR_LIB_H
#define __TAR_LIB_H

#define TAR_BLOCK_SIZE 512

#ifdef __cplusplus
extern "C" {
#endif

struct tar_header {
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
};

#ifdef __cplusplus
}
#endif


#endif // !__TAR_LIB_H