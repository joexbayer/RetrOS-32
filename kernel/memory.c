/**
 * @file memory.c
 * @author Joe Bayer (joexbayer)
 * @brief A primitiv memory allocation program, using chunks with size of 4096
 * @version 0.1
 * @date 2022-06-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <memory.h>
#include <screen.h>
#include <timer.h>
#include <sync.h>

#define MEM_START 0x100000
#define MEM_END 0xEFFFFF
#define MEM_CHUNK 0x1000
#define CHUNKS_SIZE 3200

static mutex_t mem_lock;
struct mem_chunk chunks[CHUNKS_SIZE];
uint16_t chunks_used = 0;

/* prototypes */
void init_memory();
void* alloc(uint16_t size);
void free(void* ptr);

/* Helper functions */
int _check_chunks(int i, int chunks_needed)
{
	for (int j = 0; j < chunks_needed; j++)
	{
		if(chunks[i+j].status != FREE)
		{
			return -1;
		}
	}

	return 1;
}

void print_memory_status()
{	
	scrcolor_set(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
	scrprintf(0,0, " Time: %d   Memory: %d/%d used. %d/%d chunks    ", get_time(), (chunks_used*MEM_CHUNK), CHUNKS_SIZE*MEM_CHUNK, chunks_used, CHUNKS_SIZE);
	scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
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
	if(size == 0) return NULL;
	acquire(&mem_lock);
	int chunks_needed = size / MEM_CHUNK;

	if(!chunks_needed) chunks_needed = 1;
	for (int i = 0; i < CHUNKS_SIZE; i++)
	{
		if(chunks[i].status == FREE)
		{	
			int ret = _check_chunks(i, chunks_needed);
			if(!ret) break;
			
			/* Found enough continious chunks for size. */
			for (int j = 0; j < chunks_needed; j++)
			{
				chunks[i+j].status = USED;
			}
			
			chunks[i].chunks_used = chunks_needed;
			chunks_used += chunks_needed;

			release(&mem_lock);
			return chunks[i].from;
		}	
	}

	release(&mem_lock);
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
	if(ptr == NULL) return;

	acquire(&mem_lock);
	for (int i = 0; i < CHUNKS_SIZE; i++)
	{
		if(chunks[i].from == ptr)
		{
			if(chunks[i].status != USED) return; /* Tried to free memory not used. */

			int used = chunks[i].chunks_used;
			for (int j = 0; j < used; j++)
			{
				chunks[i+j].status = FREE;
				chunks[i+j].chunks_used = 0;
			}
			chunks_used -= used;
			release(&mem_lock);
			return;
		}
	}
	release(&mem_lock);	
}

/**
 * Initializes all memory chunks and sets them to be free.
 * 
 * @return void
 */
void init_memory()
{
	mutex_init(&mem_lock);

	uint32_t mem_position = MEM_START;
	for (int i = 0; i < CHUNKS_SIZE; i++)
	{
		chunks[i].size = MEM_CHUNK;
		chunks[i].from = (uint32_t*) mem_position;
		chunks[i].chunks_used = 0;
		chunks[i].status = FREE;

		mem_position += MEM_CHUNK;
	}
}