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
#include <terminal.h>
#include <timer.h>
#include <sync.h>

#define MEM_START 0x300000
#define MEM_END 0xEFFFFF
#define MEM_CHUNK 0x400
#define CHUNKS_SIZE (MEM_END-MEM_START)/MEM_CHUNK

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
		if(chunks[i+j].status != FREE)
			return -1;

	return 1;
}

void print_mem()
{
	static const char* SIZES[] = { "B", "kB", "MB", "GB" };
    int div_used = 0;
	int div_main = 0;

	int used = (chunks_used*MEM_CHUNK);
	int main = CHUNKS_SIZE*MEM_CHUNK;

    while (used >= 1024 && div_used < (sizeof SIZES / sizeof *SIZES)) {
        div_used++;   
        used /= 1024;
    }

	while (main >= 1024 && div_main < (sizeof SIZES / sizeof *SIZES)) {
        div_main++;   
        main /= 1024;
    }
	
	if(div_main > 3 || div_main > 3)
		return;

	scrprintf(30, 0, "MEM: %d%s / %d%s", used ,SIZES[div_used], main, SIZES[div_main]);
}

void print_memory_status()
{	
	struct time* time = get_datetime();
	scrcolor_set(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
	for (int i = 0; i < SCREEN_WIDTH; i++)
		scrput(i, 0, 205, VGA_COLOR_LIGHT_GREY);

	scrput(50, 0, 203, VGA_COLOR_LIGHT_GREY);
	
	scrprintf(1,0, "NETOS");
	scrprintf(SCREEN_WIDTH-17, 0, "%d:%d:%d %d/%d/%d", time->hour, time->minute, time->second, time->day, time->month, time->year);

	print_mem();

	scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/* implementation */

/**
 * @brief Allocates sequential chunks with fixed size 4Kb each.
 * Will allocate multiple chunks if needed.
 * 
 * @param uint16_t size, how much memory is needed (Best if 4Kb aligned.).
 * @return void* to memory location. NULL if not enough continious chunks.
 */
void* alloc(uint16_t size)
{
	if(size == 0) return NULL;
	acquire(&mem_lock);

	int chunks_needed = 0;
	while(chunks_needed*MEM_CHUNK < size)
		chunks_needed++;
	

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
 * @brief Will free all chunks associated with chunk pointed to by ptr. 
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
			if(chunks[i].status != USED) 
			{
				release(&mem_lock);
				return; /* Tried to free memory not used. */
			}

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
 * @brief Initializes all memory chunks and sets them to be free.
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