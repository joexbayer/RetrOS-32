#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

enum {
	USED,
	FREE
};

enum {
  PRESENT = 1,    /* present */
  RW = 2,    	/* read/ write */
  USER = 4,    /* user/ supervisor */
  WRITE_THROUGH = 8,  /* page write-through */
  CACHE_DISABLE = 16,  /* page cache disable */
  ACCESSED = 32,    /* accessed */
  DIRTY = 64,    /* dirty */

  PAGE_DIRECTORY_BITS = 22,  /* position of page dir index */
  PAGE_TABLE_BITS = 12,  /* position of page table index */
  PAGE_DIRECTORY_MASK = 0xffc00000,  /* extracts page dir index */
  PAGE_TABLE_MASK = 0x000003ff,  /* extracts page table index */
  PAGE_SIZE = 0x1000,  /* 4096 bytes */
  PAGE_MASK = 0xfff,  /* extracts 10 lsb */

  /* size of a page table in bytes */
  PAGE_TABLE_SIZE = (1024 * 4096 - 1)
};


struct mem_chunk
{
	uint16_t size;
	uint32_t* from;
	uint16_t chunks_used; /* Used when freeing */
	uint8_t status;
};

void init_memory();
void* alloc(uint16_t size);
void print_memory_status();
void free(void* ptr);

extern uint32_t* kernel_page_dir;
void init_paging();

#endif