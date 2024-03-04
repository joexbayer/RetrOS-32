#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

/* ELF Header for 32-bit */
typedef struct {
    unsigned char   e_ident[16]; /* Magic number and other info */
    uint16_t        e_type;      /* Object file type */
    uint16_t        e_machine;   /* Architecture */
    uint32_t        e_version;   /* Object file version */
    uint32_t        e_entry;     /* Entry point virtual address */
    uint32_t        e_phoff;     /* Program header table file offset */
    uint32_t        e_shoff;     /* Section header table file offset */
    uint32_t        e_flags;     /* Processor-specific flags */
    uint16_t        e_ehsize;    /* ELF header size in bytes */
    uint16_t        e_phentsize; /* Program header table entry size */
    uint16_t        e_phnum;     /* Program header table entry count */
    uint16_t        e_shentsize; /* Section header table entry size */
    uint16_t        e_shnum;     /* Section header table entry count */
    uint16_t        e_shstrndx;  /* Section header string table index */
} Elf32_Ehdr;

/* ELF Section Header for 32-bit */
typedef struct {
    uint32_t   sh_name;      /* Section name (string tbl index) */
    uint32_t   sh_type;      /* Section type */
    uint32_t   sh_flags;     /* Section flags */
    uint32_t   sh_addr;      /* Section virtual addr at execution */
    uint32_t   sh_offset;    /* Section file offset */
    uint32_t   sh_size;      /* Section size in bytes */
    uint32_t   sh_link;      /* Link to another section */
    uint32_t   sh_info;      /* Additional section information */
    uint32_t   sh_addralign; /* Section alignment */
    uint32_t   sh_entsize;   /* Entry size if section holds table */
} Elf32_Shdr;

/* ELF Program Header for 32-bit */
typedef struct {
    uint32_t   p_type;   /* Segment type */
    uint32_t   p_offset; /* Segment file offset */
    uint32_t   p_vaddr;  /* Segment virtual address */
    uint32_t   p_paddr;  /* Segment physical address */
    uint32_t   p_filesz; /* Segment size in file */
    uint32_t   p_memsz;  /* Segment size in memory */
    uint32_t   p_flags;  /* Segment flags */
    uint32_t   p_align;  /* Segment alignment */
} Elf32_Phdr;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ELF file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    Elf32_Ehdr ehdr;
    if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        perror("Error reading ELF header");
        close(fd);
        return EXIT_FAILURE;
    }

    /* Print ELF Header Information */
    printf("ELF Header:\n");
    printf("  Type: %u\n", ehdr.e_type);
    printf("  Machine: %u\n", ehdr.e_machine);
    printf("  Version: %u\n", ehdr.e_version);
    printf("  Entry point address: 0x%x\n", ehdr.e_entry);
    printf("  Start of program headers: %u (bytes into file)\n", ehdr.e_phoff);
    printf("  Start of section headers: %u (bytes into file)\n", ehdr.e_shoff);
    printf("  Flags: %u\n", ehdr.e_flags);
    printf("  Size of this header: %u (bytes)\n", ehdr.e_ehsize);
    printf("  Size of program headers: %u (bytes)\n", ehdr.e_phentsize);
    printf("  Number of program headers: %u\n", ehdr.e_phnum);
    printf("  Size of section headers: %u (bytes)\n", ehdr.e_shentsize);
    printf("  Number of section headers: %u\n", ehdr.e_shnum);
    printf("  Section header string table index: %u\n\n", ehdr.e_shstrndx);

    /* Read and Print Program Headers */
    Elf32_Phdr phdr;
    printf("Program Headers:\n");
    for (int i = 0; i < ehdr.e_phnum; i++) {
        lseek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
        if (read(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) {
            perror("Error reading program header");
            close(fd);
            return EXIT_FAILURE;
        }

        printf("  Program Header %d: \n", i);
        printf("    Type: %u\n", phdr.p_type);
        printf("    Offset: 0x%x\n", phdr.p_offset);
        printf("    Virtual Address: 0x%x\n", phdr.p_vaddr);
        printf("    Physical Address: 0x%x\n", phdr.p_paddr);
        printf("    File Size: %u\n", phdr.p_filesz);
        printf("    Memory Size: %u\n", phdr.p_memsz);
        printf("    Flags: %u\n", phdr.p_flags);
        printf("    Align: %u\n\n", phdr.p_align);
    }

    /* Read and Print Section Headers */
    Elf32_Shdr shdr;
    printf("Section Headers:\n");
    for (int i = 0; i < ehdr.e_shnum; i++) {
        lseek(fd, ehdr.e_shoff + i * ehdr.e_shentsize, SEEK_SET);
        if (read(fd, &shdr, sizeof(shdr)) != sizeof(shdr)) {
            perror("Error reading section header");
            close(fd);
            return EXIT_FAILURE;
        }

        printf("  Section Header %d:\n", i);
        printf("    Name: %u\n", shdr.sh_name);
        printf("    Type: %u\n", shdr.sh_type);
        printf("    Flags: 0x%x\n", shdr.sh_flags);
        printf("    Address: 0x%x\n", shdr.sh_addr);
        printf("    Offset: 0x%x\n", shdr.sh_offset);
        printf("    Size: %u\n", shdr.sh_size);
        printf("    Link: %u\n", shdr.sh_link);
        printf("    Info: %u\n", shdr.sh_info);
        printf("    Address alignment: %u\n", shdr.sh_addralign);
        printf("    Entry size: %u\n\n", shdr.sh_entsize);
    }

    close(fd);
    return EXIT_SUCCESS;
}
