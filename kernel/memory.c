#include <memory.h>
#include <screen.h>

#define MEM_START 0x100000
#define MEM_END 0xEFFFFF

#define MEM_CHUNK 0x1000

#define CHUNKS_SIZE 300

mem_chunk_t chunks[CHUNKS_SIZE];
uint16_t chunks_used = 0;

/* prototypes */
void init_memory();
void* alloc(uint16_t size);
void free(void* ptr);


/* Helper functions */
int _alloc_chunks(int i, int chunks_needed)
{
	for (size_t j = 0; j < chunks_needed; j++)
	{
		if(chunks[i+j].status != FREE)
		{
			return -1;
		}
	}

	return 1;
}

void __print_memory_status()
{
	scrprintf(0,0, "Memory: %d/%d used. %d/%d chunks", (chunks_used*MEM_CHUNK), (CHUNKS_SIZE-chunks_used)*MEM_CHUNK, chunks_used, CHUNKS_SIZE);
}

/* implementation */

/**
 * Allocates sequential chunks with fixed size 4Kb each.
 * Will allocate multiple chunks if needed.
 * 
 * @param uint16_t size, how much memory is needed (Best if 4Kb aligned.).
 * @return void* to memory location. NULL if not enough continious chunks.
 */
void* alloc(uint16_t size)
{
	CLI();

	if(size == 0) NULL;
	int chunks_needed = size / MEM_CHUNK;

	if(!chunks_needed) chunks_needed = 1;
	for (size_t i = 0; i < CHUNKS_SIZE; i++)
	{
		if(chunks[i].status == FREE)
		{	
			int ret = _alloc_chunks(i, chunks_needed);
			if(!ret) break;
			
			/* Found enough continious chunks for size. */
			for (size_t j = 0; j < chunks_needed; j++)
			{
				chunks[i+j].status = USED;
			}
			
			chunks[i].chunks_used = chunks_needed;
			chunks_used += chunks_needed;
			__print_memory_status();

			STI();
			return chunks[i].from;
		}	
	}
	STI();
	return NULL;
}
/**
 * Will free all chunks associated with chunk pointed to by ptr. 
 * 
 * @param void* ptr, pointer to memory to free.
 * @return void
 */
void free(void* ptr)
{
	CLI();
	if(ptr == NULL) return -1;
	for (size_t i = 0; i < CHUNKS_SIZE; i++)
	{
		if(chunks[i].from == ptr)
		{
			if(chunks[i].status != USED) return; /* Tried to free memory not used. */

			int used = chunks[i].chunks_used;
			for (size_t j = 0; j < used; j++)
			{
				chunks[i+j].status = FREE;
				chunks[i+j].chunks_used = 0;
			}
			chunks_used -= used;
			__print_memory_status();
			STI();
			return;
		}
	}
	STI();	
}

/**
 * Initializes all memory chunks and sets them to be free.
 * 
 * @return void
 */
void init_memory()
{
	uint32_t mem_position = MEM_START;
	for (size_t i = 0; i < CHUNKS_SIZE; i++)
	{
		chunks[i].size = MEM_CHUNK;
		chunks[i].from = mem_position;
		chunks[i].chunks_used = 0;
		chunks[i].status = FREE;

		mem_position += MEM_CHUNK;
	}
	__print_memory_status();
}