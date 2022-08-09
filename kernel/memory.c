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
#include <serial.h>
#include <timer.h>
#include <sync.h>
#include <diskdev.h>

#include <bitmap.h>

enum ASCII {
	ASCII_BLOCK = 219,
	ASCII_HORIZONTAL_LINE = 205,
	ASCII_VERTICAL_LINE = 179,
	ASCII_DOWN_INTERSECT = 203
};

#define MEM_START 0x300000
#define MEM_END 0x400000
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
    uint32_t div_used = 0;
	uint32_t div_main = 0;

	int disk_used = disk_size();
	uint32_t div_disk = 0;

	int used = (chunks_used*MEM_CHUNK);
	int main = CHUNKS_SIZE*MEM_CHUNK;

    while (used >= 1024 && div_used < (sizeof SIZES / sizeof *SIZES)) {
        div_used++;   
        used /= 1024;
    }

	while (disk_used >= 1024 && div_disk < (sizeof SIZES / sizeof *SIZES)) {
        div_disk++;   
        disk_used /= 1024;
    }

	while (main >= 1024 && div_main < (sizeof SIZES / sizeof *SIZES)) {
        div_main++;   
        main /= 1024;
    }
	
	if(div_main > 3 || div_main > 3)
		return;


	scrprintf(18, 0, "DISK: %d%s", disk_used ,SIZES[div_disk]);
	
	scrprintf(40, 0, "MEM: %d%s / %d%s", used ,SIZES[div_used], main, SIZES[div_main]);
}

void print_memory_status()
{	
	struct time* time = get_datetime();

	for (int x = 0; x < SCREEN_HEIGHT; x++)
		scrput(0, 0+x, ASCII_VERTICAL_LINE, VGA_COLOR_LIGHT_GREY);

	for (int x = 0; x < SCREEN_HEIGHT; x++)
		scrput(SCREEN_WIDTH-1, 0+x, ASCII_VERTICAL_LINE, VGA_COLOR_LIGHT_GREY);

	scrput(0, SCREEN_HEIGHT-1, 192, VGA_COLOR_LIGHT_GREY);
	scrput(SCREEN_WIDTH-1, SCREEN_HEIGHT-1, 217, VGA_COLOR_LIGHT_GREY);

	for (int x = 1; x < SCREEN_WIDTH-1; x++)
		scrput(x, SCREEN_HEIGHT-1, 196, VGA_COLOR_LIGHT_GREY);


	scrcolor_set(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
	for (int i = 0; i < SCREEN_WIDTH-1; i++)
		scrput(i, 0, 205, VGA_COLOR_LIGHT_GREY);

	//scrput(50, 0, 203, VGA_COLOR_LIGHT_GREY);
	scrput(0, 0, 213, VGA_COLOR_LIGHT_GREY);
	scrput(SCREEN_WIDTH-1, 0, 184, VGA_COLOR_LIGHT_GREY);
	
	scrprintf(2,0, "NETOS");
	scrprintf(SCREEN_WIDTH-18, 0, "%d:%d:%d %d/%d/%d", time->hour, time->minute, time->second, time->day, time->month, time->year);

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

	dbgprintf("Mem data size: %d\n", sizeof(struct mem_chunk)*CHUNKS_SIZE);

	dbgprintf("[MEM] Memory initilized.\n");
}


/*  PAGIN / VIRTUAL MEMORY SECTION */
#define TOTAL_PAGES 2
uint32_t* kernel_page_dir = NULL;
bitmap_t page_bitmap;

#define TABLE_INDEX(vaddr) ((vaddr >> PAGE_TABLE_BITS) & PAGE_TABLE_MASK)
#define DIRECTORY_INDEX(vaddr) ((vaddr >> PAGE_DIRECTORY_BITS) & PAGE_TABLE_MASK)

uint32_t* alloc_page()
{
	int bit = get_free_bitmap(page_bitmap, TOTAL_PAGES);
	uint32_t* paddr = (uint32_t*) (0x200000 + (bit * PAGE_SIZE));
	memset(paddr, 0, PAGE_SIZE);

	return paddr;
}


static inline void table_set(uint32_t* page_table, uint32_t vaddr, uint32_t paddr, int access)
{
	page_table[TABLE_INDEX(vaddr)] = (paddr & ~PAGE_MASK) | access;
}


static inline void directory_insert_table(uint32_t* directory, uint32_t vaddr, uint32_t* table, int access)
{
  	directory[DIRECTORY_INDEX(vaddr)] = (((uint32_t) table) & ~PAGE_MASK) | access;
}

void init_paging()
{
	page_bitmap = create_bitmap(TOTAL_PAGES);


	kernel_page_dir = alloc_page();
	uint32_t* kernel_page_table = alloc_page();


	int permissions = PRESENT | RW | USER;
	for (int addr = 0; addr < 640 * 1024; addr += PAGE_SIZE)
    	table_set(kernel_page_table, addr, addr, permissions);
	
	table_set(kernel_page_table, (uint32_t) 0xB8000, (uint32_t) 0xB8000, permissions);
	table_set(kernel_page_table, (uint32_t) 0xB9000, (uint32_t) 0xB9000, permissions);

	
	//directory_insert_table(kernel_page_dir, 0x100000, kernel_page_table_memory, permissions);
	directory_insert_table(kernel_page_dir, 0, kernel_page_table, permissions); 
}