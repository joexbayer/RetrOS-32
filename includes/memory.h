#ifndef MEMORY_H
#define MEMORY_H

#include <util.h>

enum {
	USED,
	FREE
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
void free(void* ptr);

#endif