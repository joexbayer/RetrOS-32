#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

enum {
	USED,
	FREE
};


extern uint32_t* kernel_page_dir;

struct mem_chunk
{
	uint16_t size;
	uint32_t* from;
	uint16_t chunks_used; /* Used when freeing */
	uint8_t status;
};

enum {
  /* Main permission bits. */
  PRESENT = 1,
  READ_WRITE = 2,
  USER = 4,

  WRITE_THROUGH = 8,
  ACCESSED = 32,

  PAGE_DIRECTORY_BITS = 22,
  PAGE_TABLE_BITS = 12,
  PAGE_TABLE_MASK = 0x000003ff,
  PAGE_SIZE = 0x1000,
  PAGE_MASK = 0xfff,
};

#define TABLE_INDEX(vaddr) ((vaddr >> PAGE_TABLE_BITS) & PAGE_TABLE_MASK)
#define DIRECTORY_INDEX(vaddr) ((vaddr >> PAGE_DIRECTORY_BITS) & PAGE_TABLE_MASK)

void init_memory();
void* alloc(uint16_t size);
void print_memory_status();
void free(void* ptr);

void driver_mmap(uint32_t addr, int size);


void load_page_directory();
void enable_paging();
void init_paging();

#endif