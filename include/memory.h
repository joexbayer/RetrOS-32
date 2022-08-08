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

void init_memory();
void* alloc(uint16_t size);
void print_memory_status();
void free(void* ptr);

#endif