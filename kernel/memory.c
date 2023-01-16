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

/**
 * Individual Process Memory Map:
 * 0x100000 - 0x200000: permanents
 * 0x200000 - 0x300000: pages
 * 0x300000 - 0x400000: dynamic
 * 
 * Total Memory map:
 * 
 * End		0x1600000
 * 	
 * 		    ~ for processes 500kb each, total of 24 processes.
 * 	
 * 			0x0400000 (kernel end)
 * 
 * 			0x0300000 (page end / Kernel start)
 * 
 * 			0x0200000 (Permanent end / Page start)
 * 
 * Start 	0x0100000 (Permanent start)
 */


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

static int memory_process_used = 0;

/* prototypes */
void init_memory();
void* kalloc(int size);
void kfree(void* ptr);

/* Helper functions */
inline int _check_chunks(int i, int chunks_needed)
{
	for (int j = 0; j < chunks_needed; j++)
		if(chunks[i+j].status != FREE)
			return -1;

	return 1;
}

void memory_total_usage()
{
	//twritef("\nTotal Memory Usage:\n\n");
	//twritef("Permanent: %d/%d (%d% )\n", memory_permanent_ptr-PERMANENT_MEM_START, PERMANENT_MEM_END-PERMANENT_MEM_START, ((memory_permanent_ptr-PERMANENT_MEM_START)/(PERMANENT_MEM_END-PERMANENT_MEM_START))*100);
	//twritef("Dynamic: %d/%d (%d% )\n", (chunks_used*MEM_CHUNK), CHUNKS_SIZE*MEM_CHUNK, ((chunks_used*MEM_CHUNK)/(CHUNKS_SIZE*MEM_CHUNK))*100);
	//twritef("Pages: %d/%d (%d% )\n", used_pages, TOTAL_PAGES, (used_pages/TOTAL_PAGES)*100);
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


int memory_dynamic_usage()
{
	return chunks_used; 
}

int memory_dynamic_total()
{
	return CHUNKS_SIZE; 
}

int memory_permanent_usage()
{
	return (memory_permanent_ptr-PERMANENT_MEM_START)/MEM_CHUNK;
}

int memory_permanent_total()
{
	return (PERMANENT_MEM_END-PERMANENT_MEM_START)/MEM_CHUNK;
}

int memory_process_total()
{
	return 0x100000*12;
}
int memory_process_usage()
{
	return memory_process_used;
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


void* __alloc_internal(int size)
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
			memset((char*)chunks[i].from, 0, size);
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
void* kalloc(int size)
{
	void* ret = __alloc_internal(size);
	if(ret == NULL)
		return NULL;
	
	dbgprintf("[MEMORY] %s Allocating %d bytes of data (%d/%d)\n", current_running->name, size, (chunks_used*MEM_CHUNK), MEM_CHUNK*CHUNKS_SIZE);
	//memory_register_alloc(current_running->name, size);

	return ret;
}
/**
 * @brief Will free all chunks associated with chunk pointed to by ptr. 
 * 
 * @param void* ptr, pointer to memory to free.
 * @return void
 */
void kfree(void* ptr)
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

#define MEMORY_PROCESS_SIZE 500*1024

void free(void* ptr)
{
	if(ptr == current_running->allocations->address){
		struct allocation* next = current_running->allocations;
		current_running->allocations = current_running->allocations->next;
		current_running->used_memory -= next->size;
		memory_process_used -= next->size;
		kfree(next);
		return;
	}

	struct allocation* iter = current_running->allocations;
	while(iter->next != NULL){
		if(iter->next->address == ptr){
			
			struct allocation* save = iter->next;
			iter->next = iter->next->next;
			current_running->used_memory -= save->size;
			memory_process_used -= save->size;
			kfree(save);
			return;
		}
	}
}

void* malloc(int size)
{
	if(current_running->allocations == NULL){
		struct allocation* allocation = kalloc(sizeof(struct allocation));
		allocation->address = 0x400000 + MEMORY_PROCESS_SIZE*current_running->pid;
		allocation->size = size;
		allocation->next = NULL;

		current_running->allocations = allocation;
		current_running->used_memory += size;
		
		memory_process_used += size;
		return (void*) allocation->address;
	}

	struct allocation* iter = current_running->allocations;
	while(iter->next != NULL){
		if(iter->next->address - (iter->address+iter->size) >= size){
			/* Found spot for allocation */
			struct allocation* new = kalloc(sizeof(struct allocation));
			new->address = iter->address+iter->size;
			new->size = size;
			new->next = NULL;

			struct allocation* next = iter->next;
			iter->next = new;
			new->next = next;
			current_running->used_memory += size;
			memory_process_used += size;
			return (void*) new->address;
		}
		iter = iter->next;
	}

	if(iter->address+iter->size+size >= (0x400000 + MEMORY_PROCESS_SIZE*current_running->pid + MEMORY_PROCESS_SIZE))
		return NULL;

	struct allocation* new = kalloc(sizeof(struct allocation));
	new->address = iter->address+iter->size;
	new->size = size;
	new->next = NULL;

	iter->next = new;
	memory_process_used += size;
	return (void*) new->address;
}

void* calloc(int size, int val)
{
	void* m = malloc(size);
	if(m == NULL)	
		return NULL;

	memset(m, val, size);
	return m;
}


/* per process memory allocator total 512kb with minimum of 128 bytes per allocation. */


/*  PAGIN / VIRTUAL MEMORY SECTION */

uint32_t* alloc_page()
{
	int bit = get_free_bitmap(page_bitmap, TOTAL_PAGES);
	uint32_t* paddr = (uint32_t*) (0x200000 + (bit * PAGE_SIZE));
	memset(paddr, 0, PAGE_SIZE);
	used_pages++;

	return paddr;
}

int memory_pages_total()
{
	return TOTAL_PAGES;
}

int memory_pages_usage()
{
	return used_pages;
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

/**
 * @brief 
 * 
 * @param pcb 
 * @param data 
 * @param size 
 * 
 * Process memory map
 * 	
 * STACK 		0xEFFFFFF0
 * 				
 * HEAP 		0x1001000?
 * 				~ 0x1000 (8kib)
 * DATA 		0x1000000
 * 
 */
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

	/* Dynamic per process memory */
	int start = 0x400000 + MEMORY_PROCESS_SIZE*pcb->pid;
	uint32_t* kernel_page_table_memory = alloc_page();
	for (int addr = start; addr < start+MEMORY_PROCESS_SIZE; addr += PAGE_SIZE)
		table_set(kernel_page_table_memory, addr, addr, permissions);

	dbgprintf("[INIT PROCESS] Mapped dynamic memory %x to %x\n", start, start);

	/* Insert page and data tables in directory. */
	directory_insert_table(process_directory, 0x1000000, process_data_table, permissions); 
	directory_insert_table(process_directory, start, kernel_page_table_memory, permissions);
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
	for (int addr = 0; addr < 0x400000; addr += PAGE_SIZE)
		table_set(kernel_page_table, addr, addr, permissions);

	for (int i = 1; i < 7; i++)
	{
		uint32_t* kernel_page_table_memory = alloc_page();
		for (int addr = 0x400000*i; addr < 0x400000*(i+1); addr += PAGE_SIZE)
			table_set(kernel_page_table_memory, addr, addr, permissions);

		directory_insert_table(kernel_page_dir, 0x400000*i, kernel_page_table_memory, permissions);
	}
	
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