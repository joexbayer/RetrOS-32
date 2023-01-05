/**
 * @file memory.c
 * @author Joe Bayer (joexbayer)
 * @brief A primitiv memory allocation program and virtual memory functions.
 * @version 0.1
 * @date 2022-06-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <memory.h>
#include <screen.h>
#include <serial.h>
#include <rtc.h>
#include <sync.h>
#include <diskdev.h>
#include <hashmap.h>
#include <vbe.h>
#include <terminal.h>

#include <bitmap.h>

enum ASCII {
	ASCII_BLOCK = 219,
	ASCII_HORIZONTAL_LINE = 205,
	ASCII_VERTICAL_LINE = 179,
	ASCII_DOWN_INTERSECT = 203
};

/**
 * Memory Map:
 * 0x100000 - 0x200000: permanents
 * 0x200000 - 0x300000: pages
 * 0x300000 - 0x400000: dynamic
 */

#define PERMANENT_MEM_START 0x100000
#define PERMANENT_MEM_END 0x200000
#define MEM_START 0x300000
#define MEM_END 0x400000
#define MEM_CHUNK 0x400
#define CHUNKS_SIZE (MEM_END-MEM_START)/MEM_CHUNK

/* Virtual Memory*/
#define TOTAL_PAGES ((0x300000-0x200000) / PAGE_SIZE)
uint32_t* kernel_page_dir = NULL;
static bitmap_t page_bitmap;
static int used_pages = 0;

/* Dynamic Memory */
static mutex_t mem_lock;
struct mem_chunk chunks[CHUNKS_SIZE]; /* TODO: convert to bitmap */
static struct hashmap memmory_hasmap;
uint16_t chunks_used = 0;
static uint32_t memory_permanent_ptr = PERMANENT_MEM_START;

/* prototypes */
void init_memory();
void* alloc(int size);
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
	struct time time;
	get_current_time(&time);

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
	scrprintf(SCREEN_WIDTH-18, 0, "%d:%d:%d %d/%d/%d", time.hour, time.minute, time.second, time.day, time.month, time.year);

	print_mem();

	scrcolor_set(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void memory_total_usage()
{
	twritef("\nTotal Memory Usage:\n\n");
	twritef("Permanent: %d/%d (%d% )\n", memory_permanent_ptr-PERMANENT_MEM_START, PERMANENT_MEM_END-PERMANENT_MEM_START, ((memory_permanent_ptr-PERMANENT_MEM_START)/(PERMANENT_MEM_END-PERMANENT_MEM_START))*100);
	twritef("Dynamic: %d/%d (%d% )\n", (chunks_used*MEM_CHUNK), CHUNKS_SIZE*MEM_CHUNK, ((chunks_used*MEM_CHUNK)/(CHUNKS_SIZE*MEM_CHUNK))*100);
	twritef("Pages: %d/%d (%d% )\n", used_pages, TOTAL_PAGES, (used_pages/TOTAL_PAGES)*100);

}

int memory_get_usage(char* name)
{
	return hashmap_get(&memmory_hasmap, name);
}

void memory_register_alloc(char* name, int size)
{
		int current = hashmap_get(&memmory_hasmap, name);
		if(current == -1){
			hashmap_put(&memmory_hasmap, name, size);
			return;
		}

		hashmap_add(&memmory_hasmap, name, size);
}

/* implementation */

/**
 * @brief Permanent allocation (no free)
 * 
 */
void* palloc(int size)
{
	if(memory_permanent_ptr + size > PERMANENT_MEM_END){
		dbgprintf("[WARNING] Out of permanent memory!\n");
		return NULL;
	}
	uint32_t new = memory_permanent_ptr + size;
	memory_permanent_ptr += size;

	return (void*) new;
}


void* __alloc_internal(uint16_t size)
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
 * @brief Allocates sequential chunks with fixed size 4Kb each.
 * Will allocate multiple chunks if needed.
 * 
 * @param uint16_t size, how much memory is needed (Best if 4Kb aligned.).
 * @return void* to memory location. NULL if not enough continious chunks.
 */
void* alloc(int size)
{
	void* ret = __alloc_internal(size);
	if(ret == NULL)
		return NULL;
	
	dbgprintf("[MEMORY] %s Allocating %d bytes of data\n", current_running->name, size);
	memory_register_alloc(current_running->name, size);

	return ret;
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

uint32_t* alloc_page()
{
	int bit = get_free_bitmap(page_bitmap, TOTAL_PAGES);
	uint32_t* paddr = (uint32_t*) (0x200000 + (bit * PAGE_SIZE));
	memset(paddr, 0, PAGE_SIZE);
	used_pages++;

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

void driver_mmap(uint32_t addr, int size)
{
	int permissions = PRESENT | READ_WRITE;
	uint32_t* kernel_page_table_e1000 = alloc_page();
	for (int i = 0; i < size; i++)
		table_set(kernel_page_table_e1000, (uint32_t) addr+(0x1000*i), (uint32_t) addr+(0x1000*i), permissions);
	
	dbgprintf("[mmap] Page for 0x%x set\n", addr);

	directory_insert_table(kernel_page_dir,  addr, kernel_page_table_e1000, permissions);
	return;
}

void init_process_paging(struct pcb* pcb, char* data, int size)
{
	int permissions = PRESENT | READ_WRITE | USER;

	/* Allocate directory and tables for data and stack */
	uint32_t* process_directory = alloc_page();
	uint32_t* process_data_table = alloc_page();
	uint32_t* process_stack_table = alloc_page();

	/* Map the process data to a page */
	uint32_t* process_data_page = alloc_page();
	memcpy(process_data_page, data, size);
	table_set(process_data_table, 0x1000000, (uint32_t) process_data_page, permissions);
	dbgprintf("[INIT PROCESS] Mapped data 0x1000000 to %x\n", process_data_page);

	/* Map the process stack to a page */
	uint32_t* process_stack_page = alloc_page();
	memset(process_stack_page, 0, PAGE_SIZE);

	table_set(process_stack_table, 0xEFFFFFF0, (uint32_t) process_stack_page, permissions);

	dbgprintf("[INIT PROCESS] Mapped data %x to %x\n",0x400000 & ~PAGE_MASK, process_stack_page);

	/* Insert page and data tables in directory. */
	directory_insert_table(process_directory, 0x1000000, process_data_table, permissions); 
	directory_insert_table(process_directory, 0xEFFFFFF0, process_stack_table, permissions);

	process_directory[0] = kernel_page_dir[0];

	dbgprintf("[INIT PROCESS] Paging done.\n");
	pcb->page_dir = (uint32_t*)process_directory;
}

void init_paging()
{
	page_bitmap = create_bitmap(TOTAL_PAGES);
	dbgprintf("[PAGIN] %d free pagable pages.\n", TOTAL_PAGES);

	kernel_page_dir = alloc_page();
	uint32_t* kernel_page_table = alloc_page();
	int permissions = PRESENT | READ_WRITE;
	for (int addr = 0; addr < 4194304; addr += PAGE_SIZE)
    	table_set(kernel_page_table, addr, addr, permissions);
	
	/**
	 * Identity map vesa color framebuffer
	 * 
	 */
	uint32_t* kernel_page_table_vesa = alloc_page();
	for (int addr = 0; addr < (vbe_info->width*vbe_info->height*(vbe_info->bpp/8))+1; addr += PAGE_SIZE)
		table_set(kernel_page_table_vesa, vbe_info->framebuffer+addr, vbe_info->framebuffer+addr, permissions);
	
	table_set(kernel_page_table, (uint32_t) 0xB8000, (uint32_t) 0xB8000, permissions);
	table_set(kernel_page_table, (uint32_t) 0xB9000, (uint32_t) 0xB9000, permissions);

	directory_insert_table(kernel_page_dir, 0, kernel_page_table, permissions);

	directory_insert_table(kernel_page_dir, vbe_info->framebuffer, kernel_page_table_vesa, permissions); 
}