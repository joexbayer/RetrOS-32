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
  /*
   * page directory/table entry bits (PMSA p.235 and p.240)
   */
  PRESENT = 1,    /* present */
  RW = 2,    	/* read/ write */
  USER = 4,    /* user/ supervisor */
  WRITE_THROUGH = 8,  /* page write-through */
  CACHE_DISABLE = 16,  /* page cache disable */
  ACCESSED = 32,    /* accessed */
  DIRTY = 64,    /* dirty */

  /* Useful sizes, bit-sizes and masks */
  PAGE_DIRECTORY_BITS = 22,  /* position of page dir index */
  PAGE_TABLE_BITS = 12,  /* position of page table index */
  PAGE_TABLE_MASK = 0x000003ff,  /* extracts page table index */
  PAGE_SIZE = 0x1000,  /* 4096 bytes */
  PAGE_MASK = 0xfff,  /* extracts 10 lsb */

  /* size of a page table in bytes */
  PAGE_TABLE_SIZE = (1024 * 4096 - 1)
};


void init_memory();
void* alloc(uint16_t size);
void print_memory_status();
void free(void* ptr);

#endif