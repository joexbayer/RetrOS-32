#include <stdio.h>
#include <stdint.h>
#include <string.h>

// Structure for the boot sector
typedef struct {
    uint8_t jmp[3];
    char oem[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectorCount;
    uint8_t fatCount;
    uint16_t rootEntryCount;
    uint16_t totalSector16;
    uint8_t media;
    uint16_t fatSize16;
    uint16_t sectorsPerTrack;
    uint16_t headCount;
    uint32_t hiddenSectorCount;
    uint32_t totalSector32;
    uint8_t driveNumber;
    uint8_t reserved1;
    uint8_t bootSignature;
    uint32_t volumeId;
    char volumeLabel[11];
    char fileSystemType[8];
    uint8_t bootCode[448];
    uint16_t bootSectorSignature;
} __attribute__((packed)) BootSector;

// Structure for a directory entry
typedef struct {
    uint8_t filename[8];                /* 8 bytes - Filename */
    uint8_t extension[3];               /* 3 bytes - Extension */
    uint8_t attributes;                 /* 1 byte - Attributes */
    uint8_t reserved;                   /* 1 byte - Reserved */
    uint8_t created_time_tenths;        /* 1 byte - Created time (tenths of a second) */
    uint16_t created_time;              /* 2 bytes - Created time */
    uint16_t created_date;              /* 2 bytes - Created date */
    uint16_t last_access_date;          /* 2 bytes - Last access date */
    uint16_t first_cluster_high;        /* 2 bytes - High 16 bits of the cluster number */
    uint16_t modified_time;             /* 2 bytes - Modified time */
    uint16_t modified_date;             /* 2 bytes - Modified date */
    uint16_t first_cluster_low;         /* 2 bytes - Low 16 bits of the cluster number */
    uint32_t file_size;                 /* 4 bytes - File size in bytes */
} __attribute__((packed)) DirectoryEntry;

int main() {
    FILE* file = fopen("fatfs.img", "r+b");
    if (file == NULL) {
        printf("Failed to open file.\n");
        return 1;
    }
    BootSector bootSector;
    fread(&bootSector, sizeof(BootSector), 1, file);

    printf("------------ Boot Sector ------------\n");

    printf("Bytes per sector: %u\n", bootSector.bytesPerSector);
    printf("Sectors per cluster: %u\n", bootSector.sectorsPerCluster);
    printf("Reserved sector count: %u\n", bootSector.reservedSectorCount);
    printf("FAT count: %u\n", bootSector.fatCount);
    printf("Root entry count: %u\n", bootSector.rootEntryCount);
    printf("Total sectors (16-bit): %u\n", bootSector.totalSector16);
    printf("Total sectors (32-bit): %u\n", bootSector.totalSector32);
    printf("Media type: 0x%02X\n", bootSector.media);
    printf("FAT size (16-bit): %u\n", bootSector.fatSize16);
    printf("Sectors per track: %u\n", bootSector.sectorsPerTrack);
    printf("Head count: %u\n", bootSector.headCount);
    printf("Hidden sector count: %u\n", bootSector.hiddenSectorCount);
    printf("Volume ID: 0x%08X\n", bootSector.volumeId);
    printf("Volume label: %.11s\n", bootSector.volumeLabel);
    printf("File system type: %.8s\n", bootSector.fileSystemType);

    printf("------------ Root Directory ------------\n");

    // Calculate the offset of the root directory
    uint32_t rootDirectoryOffset = (bootSector.reservedSectorCount + bootSector.fatCount * bootSector.fatSize16) * bootSector.bytesPerSector;

    // Seek to the root directory offset
    fseek(file, rootDirectoryOffset, SEEK_SET);

    // Read directory entries
    DirectoryEntry entry;
    int i = 0;
    while (fread(&entry, sizeof(DirectoryEntry), 1, file) == 1 && 10 > i++) {
        // Check for end of directory marker
        if (entry.filename[0] == 0x00)
            break;

        if(entry.attributes != 0x20 && entry.attributes != 0x10) continue;
        // Check if the entry is a file and not a directory
        if (entry.attributes == 0x20) {
            // Process the file entry
            printf("Name: %.8s\n", entry.filename);
            printf("Extension: %.3s\n", entry.extension);
            printf("Attributes: 0x%02X\n", entry.attributes);
            printf("File Size: %u bytes\n", entry.file_size);
            printf("first_cluster_low: %x\n", entry.first_cluster_low);
            printf("first_cluster_high: %x\n", entry.first_cluster_high);

            // Seek to the file's data in the filesystem image
            uint32_t fileDataOffset = (rootDirectoryOffset + ((entry.first_cluster_low+6) * bootSector.sectorsPerCluster * bootSector.bytesPerSector));
            fseek(file, fileDataOffset, SEEK_SET);

            // Read and display the file contents
            uint8_t fileBuffer[entry.file_size];
            fread(fileBuffer, sizeof(uint8_t), entry.file_size, file);
            printf("File Contents: (@ 0x%x)\n", fileDataOffset);
            for (uint32_t i = 0; i < entry.file_size; i++) {
                printf("%c", fileBuffer[i]);
            }
            printf("\n");
        }
    }
    /*
    fseek(file, rootDirectoryOffset + (i-1)*sizeof(DirectoryEntry), SEEK_SET);


    /*DirectoryEntry newEntry;
    strncpy((char*)newEntry.filename, "NEWDIR", 8);
    strncpy((char*)newEntry.extension, "   ", 3);
    newEntry.attributes = 0x10; // Directory attribute

    // Write the new directory entry to the file
    //fseek(file, rootDirectoryOffset, SEEK_SET);
    fwrite(&newEntry, sizeof(DirectoryEntry), 1, file);*/

    fclose(file);
    return 0;
}
