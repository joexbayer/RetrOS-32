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

uint32_t firstDataSector;

uint32_t findEmptyFATEntry(FILE* file, BootSector bootSector) {
    uint32_t fatOffset = bootSector.reservedSectorCount * bootSector.bytesPerSector;
    uint16_t fatEntry;

    for(uint32_t i = 0; i < bootSector.fatSize16 * bootSector.bytesPerSector; i += 2) {
        fseek(file, fatOffset + i, SEEK_SET);
        fread(&fatEntry, sizeof(uint16_t), 1, file);
        if(fatEntry == 0x0000) {
            return i / 2;
        }
    }
    return 0xFFFFFFFF; // Invalid cluster, means FAT is full or error
}

void initializeFAT16FileSystem(FILE *file) {
    BootSector bootSector;

    // Define default boot sector
    memset(&bootSector, 0, sizeof(BootSector));
    bootSector.bytesPerSector = 512;
    bootSector.sectorsPerCluster = 1;
    bootSector.reservedSectorCount = 1;
    bootSector.fatCount = 2;
    bootSector.rootEntryCount = 512;
    bootSector.totalSector16 = 2880;  // 1.44MB
    bootSector.media = 0xF0;
    bootSector.fatSize16 = 9;
    bootSector.sectorsPerTrack = 18;
    bootSector.headCount = 2;
    strncpy(bootSector.fileSystemType, "FAT16   ", 8);
    bootSector.bootSectorSignature = 0xAA55;

    // Write boot sector to file
    fseek(file, 0, SEEK_SET);
    fwrite(&bootSector, sizeof(BootSector), 1, file);

    uint16_t fatValue;

    // First FAT entry
    fatValue = 0xFFF8;  // Assuming media descriptor is 0xF8
    fwrite(&fatValue, sizeof(uint16_t), 1, file);

    // Second FAT entry
    fatValue = 0xFFFF;
    fwrite(&fatValue, sizeof(uint16_t), 1, file);

    // All subsequent FAT entries
    fatValue = 0x0000;
    for (int i = 2; i < bootSector.fatSize16 * bootSector.bytesPerSector / 2; i++) {
        fwrite(&fatValue, sizeof(uint16_t), 1, file);
    }

    // Duplicate for second FAT table (FAT16 usually has 2 FAT tables)
    for (int i = 0; i < bootSector.fatSize16 * bootSector.bytesPerSector / 2; i++) {
        fwrite(&fatValue, sizeof(uint16_t), 1, file);
    }

    // Create empty root directory entries
    DirectoryEntry emptyEntry;
    memset(&emptyEntry, 0, sizeof(DirectoryEntry));
    for (int i = 0; i < bootSector.rootEntryCount; i++) {
        fwrite(&emptyEntry, sizeof(DirectoryEntry), 1, file);
    }

    // For the rest of the sectors in the file (e.g., a 1.44MB floppy image),
    // we can simply fill them with zeros (indicating unused clusters).
    uint8_t zero = 0;
    for (int i = (bootSector.reservedSectorCount + bootSector.fatCount * bootSector.fatSize16 + bootSector.rootEntryCount * sizeof(DirectoryEntry) / bootSector.bytesPerSector);
         i < bootSector.totalSector16; i++) {
        for (int j = 0; j < bootSector.bytesPerSector; j++) {
            fwrite(&zero, 1, 1, file);
        }
    }
}


void allocateClusterInFAT(FILE* file, BootSector bootSector, uint32_t clusterNumber, uint16_t value) {
    uint32_t fatOffset = bootSector.reservedSectorCount * bootSector.bytesPerSector;
    fseek(file, fatOffset + clusterNumber * 2, SEEK_SET);
    fwrite(&value, sizeof(uint16_t), 1, file);
}

void writeFileInBlocks(FILE* file, BootSector bootSector, DirectoryEntry entry, uint8_t* data, uint32_t size) {
    uint32_t remaining = size;
    uint32_t offset = (entry.first_cluster_low - 2) * bootSector.sectorsPerCluster * bootSector.bytesPerSector + firstDataSector * bootSector.bytesPerSector;
    while (remaining > 0) {
        uint32_t toWrite = remaining > 512 ? 512 : remaining;
        fseek(file, offset, SEEK_SET);
        fwrite(data, 1, toWrite, file);
        data += toWrite;
        offset += toWrite;
        remaining -= toWrite;
    }
}

void createNewDirectoryEntry(FILE* file, BootSector bootSector, char* name, char* ext, uint8_t attributes, uint32_t parentDirectoryOffset) {
    DirectoryEntry newEntry;
    memset(&newEntry, 0, sizeof(DirectoryEntry));
    strncpy((char*)newEntry.filename, name, 8);
    strncpy((char*)newEntry.extension, ext, 3);
    newEntry.attributes = attributes;

    uint32_t newCluster = findEmptyFATEntry(file, bootSector);
    if (newCluster == 0xFFFFFFFF) {
        printf("FAT is full or an error occurred.\n");
        return;
    }
    allocateClusterInFAT(file, bootSector, newCluster, 0xFFFF);  // Mark cluster as end of file
    newEntry.first_cluster_low = newCluster;

    uint32_t currentOffset = parentDirectoryOffset;
    DirectoryEntry currentDirEntry;
    int foundEmptySlot = 0;
    uint32_t i; 
    // Search for an empty slot in the directory
    for (i = 0; i < bootSector.rootEntryCount; i++) {
        fseek(file, currentOffset + i * sizeof(DirectoryEntry), SEEK_SET);
        fread(&currentDirEntry, sizeof(DirectoryEntry), 1, file);

        // Check if the filename's first byte is 0x00, which indicates an unused directory entry
        if (currentDirEntry.filename[0] == 0x00) {
            foundEmptySlot = 1;
            break;
        }
    }

    if (!foundEmptySlot) {
        printf("No available directory slots found.\n");
        return;
    }

    // If an empty slot is found, write the new entry there
    fseek(file, currentOffset + i * sizeof(DirectoryEntry), SEEK_SET);
    fwrite(&newEntry, sizeof(DirectoryEntry), 1, file);
}

int createFile(FILE* file, BootSector bootSector, const char* filename, char* content) {
    // Assuming filenames are in 8.3 format (8 for the name, 3 for the extension)


      printf("Content: %s, %d\n", content, strlen(content));
    if (strlen(filename) > 12) {
        printf("Filename too long.\n");
        return 1;
    }

    char name[8] = "NEWFILE";
    char ext[3] = "TXT";

    // Locate an empty FAT entry for the new file
    uint32_t cluster = findEmptyFATEntry(file, bootSector);
    if (cluster == 0xFFFFFFFF) {
        printf("No free clusters available.\n");
        return 2;
    }

    // Update the FAT entry to mark the end of the cluster chain
    uint32_t fatOffset = bootSector.reservedSectorCount * bootSector.bytesPerSector + cluster * 2;
    uint16_t endOfChain = 0xFFFF;
    fseek(file, fatOffset, SEEK_SET);
    fwrite(&endOfChain, sizeof(uint16_t), 1, file);

    // Calculate the offset for the file data
    uint32_t dataOffset = (bootSector.reservedSectorCount + bootSector.fatCount * bootSector.fatSize16) * bootSector.bytesPerSector;
    dataOffset += (cluster - 2) * bootSector.sectorsPerCluster * bootSector.bytesPerSector;
    printf("Writing file to content to 0x%x\n", dataOffset);
    fseek(file, dataOffset, SEEK_SET);
    fwrite(content, 1, strlen(content), file);

    // Create directory entry
    DirectoryEntry newEntry;
    memset(&newEntry, 0, sizeof(DirectoryEntry));
    strncpy((char*)newEntry.filename, name, 8);
    strncpy((char*)newEntry.extension, ext, 3);
    newEntry.attributes = 0x20;  // File attribute
    newEntry.first_cluster_low = cluster;
    newEntry.file_size = strlen(content);

    // Find an empty directory entry in the root directory to store the new file metadata
    uint32_t rootOffset = (bootSector.reservedSectorCount + bootSector.fatCount * bootSector.fatSize16) * bootSector.bytesPerSector;
    uint32_t maxRootEntries = bootSector.rootEntryCount;

    DirectoryEntry existingEntry;
    int entryFound = 0;

    for (uint32_t i = 0; i < maxRootEntries; i++) {
        fseek(file, rootOffset + i * sizeof(DirectoryEntry), SEEK_SET);
        fread(&existingEntry, sizeof(DirectoryEntry), 1, file);

        if (existingEntry.filename[0] == 0x00 || existingEntry.filename[0] == 0xE5) {
            fseek(file, rootOffset + i * sizeof(DirectoryEntry), SEEK_SET);
            fwrite(&newEntry, sizeof(DirectoryEntry), 1, file);
            entryFound = 1;
            break;
        }
    }

    if (!entryFound) {
        printf("Root directory is full.\n");
        return 3;
    }

    return 0; // File successfully created
}


void processDirectory(FILE* file, BootSector bootSector, uint32_t offset, int level) {
    // Seek to the directory offset
    fseek(file, offset, SEEK_SET);
    for (int i = 0; i < level; i++){printf("\t");}
    printf("Context: @ 0x%x (%d)\n", offset, offset);

    // Read directory entries
    DirectoryEntry entry;
    int i = 0;
    while (fread(&entry, sizeof(DirectoryEntry), 1, file) == 1) {
        // Check for end of directory marker
        i++;
        printf("Entry %d\n", i);
        if (entry.filename[0] == 0x00)
            break;

        if(entry.attributes != 0x20 && entry.attributes != 0x10) continue;
        //if(entry.filename[0] == '.') continue;

        for (int i = 0; i < level; i++){printf("\t");}

        // Check if the entry is a file or a directory
        printf("Name: %.8s\n", entry.filename);
         for (int i = 0; i < level; i++){printf("\t");}
        printf("Extension: %.3s\n", entry.extension);
         for (int i = 0; i < level; i++){printf("\t");}
        printf("Attributes: 0x%02X\n", entry.attributes);
         for (int i = 0; i < level; i++){printf("\t");}
        printf("File Size: %u bytes\n", entry.file_size);

        uint32_t dataOffset = (bootSector.reservedSectorCount + bootSector.fatCount * bootSector.fatSize16) * bootSector.bytesPerSector;
        dataOffset += (entry.first_cluster_low - 2) * bootSector.sectorsPerCluster * bootSector.bytesPerSector;

        if (entry.attributes == 0x20) {
            // Process the file entry

            // Seek to the file's data in the filesystem image
            fseek(file, dataOffset, SEEK_SET);

            // Read and display the file contents
            uint8_t fileBuffer[entry.file_size];
            fread(fileBuffer, sizeof(uint8_t), entry.file_size, file);
             for (int i = 0; i < level; i++){printf("\t");}
            printf("File Contents: (@ 0x%x)\n", dataOffset);
             for (int i = 0; i < level; i++){printf("\t");}
            for (uint32_t i = 0; i < entry.file_size; i++) {
                printf("%c", fileBuffer[i]);
            }
            printf("\n");
        } else if (entry.attributes == 0x10) {
            // Process the subdirectory entry
            // Calculate the offset of the subdirectory
            uint32_t subDirectoryOffset = (offset + ((entry.first_cluster_low+6) * bootSector.sectorsPerCluster * bootSector.bytesPerSector));
            processDirectory(file, bootSector, subDirectoryOffset, level + 1);
        }
        fseek(file, offset + (i)*sizeof(DirectoryEntry), SEEK_SET);

    }
}

int main() {
    FILE* file = fopen("fatfs.img", "r+b");
    if (file == NULL) {
        printf("Failed to open file.\n");
        return 1;
    }
    BootSector bootSector;
    fread(&bootSector, sizeof(BootSector), 1, file);

    if(bootSector.bytesPerSector != 512){
        fseek(file, 0, SEEK_SET);
        printf("Initializing FAT16 file system...\n");
        initializeFAT16FileSystem(file);
    }

    fseek(file, 0, SEEK_SET);
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
    //uint32_t rootDirectoryOffset = (bootSector.reservedSectorCount + bootSector.fatCount * bootSector.fatSize16) * bootSector.bytesPerSector;

    // Seek to the root directory offset
    //fseek(file, rootDirectoryOffset, SEEK_SET);

    // Read directory entries
    // Calculate the offset of the root directory
    uint32_t rootDirectoryOffset = (bootSector.reservedSectorCount + bootSector.fatCount * bootSector.fatSize16) * bootSector.bytesPerSector;
    // Compute RootDirSectors
    uint32_t rootDirSectors = ((bootSector.rootEntryCount * 32) + (bootSector.bytesPerSector - 1)) / bootSector.bytesPerSector;

// Calculate the first data sector
    firstDataSector = bootSector.reservedSectorCount + (bootSector.fatCount * bootSector.fatSize16) + rootDirSectors;
    
    printf("%d\n", firstDataSector);
    
    processDirectory(file, bootSector, rootDirectoryOffset, 0);

    printf("Creating new directory...\n");
    createNewDirectoryEntry(file, bootSector, "NEWDIR", "   ", 0x10, rootDirectoryOffset);
    printf("Directory created.\n");

    createFile(file, bootSector, "NEWFILE.TXT", "Hello World!");

    // Re-process the directory to show the new directory
    processDirectory(file, bootSector, rootDirectoryOffset, 0);

    // printf("Creating new file...\n");
    // createNewDirectoryEntry(file, bootSector, "NEWFILE", "TXT", 0x20, rootDirectoryOffset);
    // uint8_t fileData[] = "Hello, World!";
    // writeFileInBlocks(file, bootSector, /*[You'd need to fetch the directory entry for the NEWFILE.TXT here]*/, fileData, sizeof(fileData) - 1);
    // printf("File created and written.\n");

    // // Re-process the directory to show the new file and its contents
    // processDirectory(file, bootSector, rootDirectoryOffset, 0);


    fclose(file);
    return 0;
}
