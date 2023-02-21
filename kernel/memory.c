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

#define VMEM_MAX_ADDRESS 0x1600000
#define VMEM_START_ADDRESS 0x400000
#define VMEM_TOTAL_PAGES ((VMEM_MAX_ADDRESS-VMEM_START_ADDRESS) / PAGE_SIZE)

struct virtual_memory_alloctor {
	int used_pages;
	bitmap_t pages;

	mutex_t lock;

	int (*alloc)(struct virtual_memory_allocator*);
	int (*free)(struct virtual_memory_allocator*, int page);
} vmem;

/* Virtual Memory*/
uint32_t* kernel_page_dir = NULL;

/* Dynamic Memory */
static mutex_t mem_lock;
struct mem_chunk chunks[CHUNKS_SIZE]; /* TODO: convert to bitmap */
static struct hashmap memmory_hasmap;
uint16_t chunks_used = 0;
static uint32_t memory_permanent_ptr = PERMANENT_KERNEL_MEMORY_START;

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
	return (memory_permanent_ptr-PERMANENT_KERNEL_MEMORY_START)/MEM_CHUNK;
}

int memory_permanent_total()
{
	return (PERMANENT_MEM_END-PERMANENT_KERNEL_MEMORY_START)/MEM_CHUNK;
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


static void* __kalloc_internal(int size)
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
void* kalloc(int size)
{
	void* ret = __kalloc_internal(size);
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
	uint32_t kmemory_start = KERNEL_MEMORY_START;
	for (int i = 0; i < CHUNKS_SIZE; i++)
	{
		chunks[i].size = MEM_CHUNK;
		chunks[i].from = (uint32_t*) kmemory_start;
		chunks[i].chunks_used = 0;
		chunks[i].status = FREE;

		kmemory_start += MEM_CHUNK;
	}
	/* TODO: set alloc and free */

	dbgprintf("[MEMORY] Memory initilized.\n");
}

#define MEMORY_PROCESS_SIZE 500*1024

void free(void* ptr)
{
	if((int)ptr == current_running->allocations->address){
		struct allocation* next = current_running->allocations;
		current_running->allocations = current_running->allocations->next;
		current_running->used_memory -= next->size;
		memory_process_used -= next->size;
		kfree(next);
		return;
	}

	struct allocation* iter = current_running->allocations;
	while(iter->next != NULL){
		if(iter->next->address == (int)ptr){
			
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

void memory_free_page(void* addr)
{
	if((uint32_t)addr > VMEM_MAX_ADDRESS ||  (uint32_t)addr < VMEM_START_ADDRESS)
		return;

	int bit = (((uint32_t) addr) - VMEM_START_ADDRESS) / PAGE_SIZE;
	if(bit < 0 || bit > (VMEM_TOTAL_PAGES))
		return;
	
	unset_bitmap(vmem.pages, bit);
	dbgprintf("VIRTUAL MEMORY] Free page %d at 0x%x\n", bit, addr);
}

uint32_t* memory_alloc_page()
{
	int bit = get_free_bitmap(vmem.pages, VMEM_TOTAL_PAGES);
	uint32_t* paddr = (uint32_t*) (PAGE_KERNEL_MEMORY_START + (bit * PAGE_SIZE));
	memset(paddr, 0, PAGE_SIZE);
	vmem.used_pages++;

	dbgprintf("[VIRTUAL MEMORY] Allocated page %d at 0x%x\n", bit, paddr);
	return paddr;
}

int memory_pages_total()
{
	return VMEM_TOTAL_PAGES;
}

int memory_pages_usage()
{
	return vmem.used_pages;
}


static inline void table_set(uint32_t* page_table, uint32_t vaddr, uint32_t paddr, int access)
{
	page_table[TABLE_INDEX(vaddr)] = (paddr & ~PAGE_MASK) | access;
}


static inline void directory_set(uint32_t* directory, uint32_t vaddr, uint32_t* table, int access)
{
  	directory[DIRECTORY_INDEX(vaddr)] = (((uint32_t) table) & ~PAGE_MASK) | access;
}

void driver_mmap(uint32_t addr, int size)
{
	int permissions = PRESENT | READ_WRITE;
	uint32_t* kernel_page_table_e1000 = memory_alloc_page();
	for (int i = 0; i < size; i++)
		table_set(kernel_page_table_e1000, (uint32_t) addr+(PAGE_SIZE*i), (uint32_t) addr+(PAGE_SIZE*i), permissions);
	
	dbgprintf("[mmap] Page for 0x%x set\n", addr);

	directory_set(kernel_page_dir,  addr, kernel_page_table_e1000, permissions);
	return;
}

void cleanup_process_paging(struct pcb* pcb)
{
	uint32_t directory = (uint32_t)pcb->page_dir;
	uint32_t data_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(0x1000000)] & ~PAGE_MASK;

	int size = pcb->data_size;
	int i = 0;
	while(size > 4096){
		uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(0x1000000)] & ~PAGE_MASK))[TABLE_INDEX(0x1000000+(i*4096))]& ~PAGE_MASK;
		memory_free_page((void*) data_page);
		size -= 4096;
		i++;
	}

	uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(0x1000000)] & ~PAGE_MASK))[TABLE_INDEX(0x1000000+(i*4096))]& ~PAGE_MASK;
	memory_free_page((void*) data_page);

	uint32_t stack_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(0xEFFFFFF0)] & ~PAGE_MASK;
	uint32_t stack_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(0xEFFFFFF0)] & ~PAGE_MASK))[TABLE_INDEX(0xEFFFFFF0)]& ~PAGE_MASK;

	uint32_t dynamic_mem = (uint32_t)pcb->page_dir[DIRECTORY_INDEX((0x400000 + MEMORY_PROCESS_SIZE*pcb->pid))] & ~PAGE_MASK;

	dbgprintf("[Memory] Cleaning up pages from pcb.\n");

	memory_free_page((void*) stack_page);
	memory_free_page((void*) stack_table);

	memory_free_page((void*) data_table);

	memory_free_page((void*) dynamic_mem);
	memory_free_page((void*) directory);
	/* Only cleanup pages above 1 to protect kernel table at 0 
	for (int i = 0; i < 1024; i++)
	{
		uint32_t* table = (uint32_t*) pcb->page_dir[i];
		for (int j = 0; j < 1024; j++)
			if(table[j] != 0)
				memory_free_page((void*)table[j]);

		memory_free_page(table);
	}

	memory_free_page(pcb->page_dir);*/
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
 * DATA 		0x1000000
 * 
 * Dynamic		  ~ 512kb
 */
void init_process_paging(struct pcb* pcb, char* data, int size)
{
	int permissions = PRESENT | READ_WRITE | USER;

	/* Allocate directory and tables for data and stack */
	uint32_t* process_directory = memory_alloc_page();
	uint32_t* process_data_table = memory_alloc_page();
	uint32_t* process_stack_table = memory_alloc_page();

	/* Map the process data to a page */
	int i = 0;
	dbgprintf("[INIT PROCESS] Mapping %d bytes of data\n", size);
	while (size > 4096)
	{
		uint32_t* process_data_page = memory_alloc_page();
		memcpy(process_data_page, data+(i*4096), 4096);
		table_set(process_data_table, 0x1000000+(i*4096), (uint32_t) process_data_page, permissions);
		dbgprintf("[INIT PROCESS] Mapped data 0x%x to %x\n",0x1000000+(i*4096), process_data_page);
		size -= 4096;
		i++;
	}
	
	uint32_t* process_data_page = memory_alloc_page();
	memcpy(process_data_page, data+(i*4096), size);
	table_set(process_data_table, 0x1000000+(i*4096), (uint32_t) process_data_page, permissions);
	dbgprintf("[INIT PROCESS] Mapped data 0x%x to %x\n",0x1000000+(i*4096), process_data_page);
	
	directory_set(process_directory, 0x1000000, process_data_table, permissions); 

	/* Map the process stack to a page */
	uint32_t* process_stack_page = memory_alloc_page();
	memset(process_stack_page, 0, PAGE_SIZE);
	table_set(process_stack_table, 0xEFFFFFF0, (uint32_t) process_stack_page, permissions);
	dbgprintf("[INIT PROCESS] Mapped stack %x to %x\n", 0x1000000, process_stack_page);

	/* Dynamic per process memory */
	int start = 0x400000 + MEMORY_PROCESS_SIZE*pcb->pid;
	uint32_t* kernel_page_table_memory = memory_alloc_page();
	for (int addr = start; addr < start+MEMORY_PROCESS_SIZE; addr += PAGE_SIZE)
		table_set(kernel_page_table_memory, addr, addr, permissions);

	dbgprintf("[INIT PROCESS] Mapped dynamic memory %x to %x\n", start, start);

	/* Insert page and data tables in directory. */
	directory_set(process_directory, start, kernel_page_table_memory, permissions);
	directory_set(process_directory, 0xEFFFFFF0, process_stack_table, permissions);

	process_directory[0] = kernel_page_dir[0];

	dbgprintf("[INIT PROCESS] Paging done.\n");
	pcb->page_dir = (uint32_t*)process_directory;
}

void init_paging()
{
	vmem.pages = create_bitmap(VMEM_TOTAL_PAGES);
	mutex_init(&vmem.lock);
	dbgprintf("[PAGIN] %d free pagable pages.\n", VMEM_TOTAL_PAGES);


	kernel_page_dir = memory_alloc_page();
	uint32_t* kernel_page_table = memory_alloc_page();
	int permissions = PRESENT | READ_WRITE;
	for (int addr = 0; addr < 0x400000; addr += PAGE_SIZE)
		table_set(kernel_page_table, addr, addr, permissions);

	for (int i = 1; i < 7; i++)
	{
		uint32_t* kernel_page_table_memory = memory_alloc_page();
		for (int addr = 0x400000*i; addr < 0x400000*(i+1); addr += PAGE_SIZE)
			table_set(kernel_page_table_memory, addr, addr, permissions);

		directory_set(kernel_page_dir, 0x400000*i, kernel_page_table_memory, permissions);
	}
	
	/**
	 * Identity map vesa color framebuffer
	 * 
	 */
	uint32_t* kernel_page_table_vesa = memory_alloc_page();
	for (int addr = 0; addr < (vbe_info->width*vbe_info->height*(vbe_info->bpp/8))+1; addr += PAGE_SIZE)
		table_set(kernel_page_table_vesa, vbe_info->framebuffer+addr, vbe_info->framebuffer+addr, permissions);
	
	table_set(kernel_page_table, (uint32_t) 0xB8000, (uint32_t) 0xB8000, permissions);
	table_set(kernel_page_table, (uint32_t) 0xB9000, (uint32_t) 0xB9000, permissions);

	directory_set(kernel_page_dir, 0, kernel_page_table, permissions);

	directory_set(kernel_page_dir, vbe_info->framebuffer, kernel_page_table_vesa, permissions); 
}