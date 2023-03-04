/**
 * @file memory.c
 * @author Joe Bayer (joexbayer)
 * @brief A primitiv memory allocation program and virtual memory functions.
 * @version 0.2
 * @date 2022-06-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <memory.h>
#include <serial.h>
#include <sync.h>
#include <vbe.h>
#include <bitmap.h>
#include <assert.h>

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

/* prototypes */
void init_memory();
void* kalloc(int size);
void kfree(void* ptr);
/* ... */

/* Virtual Memory*/
#define VMEM_MAX_ADDRESS 0x1600000
#define VMEM_START_ADDRESS 0x400000
#define VMEM_TOTAL_PAGES ((VMEM_MAX_ADDRESS-VMEM_START_ADDRESS) / PAGE_SIZE)
struct virtual_memory_allocator {
	int used_pages;
	bitmap_t pages;

	mutex_t lock;

	int (*alloc)(struct virtual_memory_allocator*);
	int (*free)(struct virtual_memory_allocator*, int page);
} vmem;
uint32_t* kernel_page_dir = NULL;

/**
 * @brief Permanent memory allocation scheme for memory that wont be freed.
 * Mainly by the windowservers framebuffer, and E1000's buffers.
 */
static uint32_t memory_permanent_ptr = PERMANENT_KERNEL_MEMORY_START;
void* palloc(int size)
{
	if(size <= 0)
		return NULL;

	if(memory_permanent_ptr + size > PMEM_END_ADDRESS){
		dbgprintf("[WARNING] Not enough permanent memory!\n");
		return NULL;
	}
	uint32_t new = memory_permanent_ptr + size;
	memory_permanent_ptr += size;

	return (void*) new;
}

uint32_t* vmem_get_page_table(uint32_t addr)
{
	return current_running->page_dir[DIRECTORY_INDEX(addr)] & ~PAGE_MASK;
}

static inline void table_set(uint32_t* page_table, uint32_t vaddr, uint32_t paddr, int access)
{
	page_table[TABLE_INDEX(vaddr)] = (paddr & ~PAGE_MASK) | access;
}


static inline void directory_set(uint32_t* directory, uint32_t vaddr, uint32_t* table, int access)
{
	  directory[DIRECTORY_INDEX(vaddr)] = (((uint32_t) table) & ~PAGE_MASK) | access;
}

uint32_t* memory_alloc_page()
{
	int bit = get_free_bitmap(vmem.pages, VMEM_TOTAL_PAGES);
	if(bit == -1)
		return NULL;

	uint32_t* paddr = (uint32_t*) (VMEM_START_ADDRESS + (bit * PAGE_SIZE));
	memset(paddr, 0, PAGE_SIZE);
	vmem.used_pages++;

	dbgprintf("[VIRTUAL MEMORY] Allocated page %d at 0x%x\n", bit, paddr);
	return paddr;
}

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



/* Dynamic kernel memory */
#define KERNEL_MEMORY_START 	0x300000
#define KERNEL_MEMORY_END	0x400000
#define KMEM_BLOCK_SIZE 	512
#define KMEM_BLOCKS_PER_BYTE 	8
#define KMEM_BITMAP_INDEX(addr) ((addr - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE / KMEM_BLOCKS_PER_BYTE)
#define KMEM_BITMAP_OFFSET(addr) ((addr - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE % KMEM_BLOCKS_PER_BYTE)

/* need to add static bitmap as, bitmap_t uses kalloc */
static uint8_t __kmemory_bitmap[(KERNEL_MEMORY_END - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE / KMEM_BLOCKS_PER_BYTE];
static mutex_t __kmemory_lock;
static uint32_t __kmemory_used = 0;

/**
 * @brief Allocates sequential chunks of fixed size (4KB each) from a region of kernel memory.
 * 
 * This function acquires a lock to ensure thread safety, and then searches the bitmap of kernel memory for a
 * contiguous region of free blocks that is large enough to accommodate the requested memory size. If such a region
 * is found, the function marks the corresponding blocks as used in the bitmap, writes the size of the allocated
 * block to a metadata block, and returns a pointer to the allocated memory block. If no contiguous region of memory
 * is found, the function returns NULL.
 * 
 * @param size The amount of memory to allocate, in bytes. It is recommended that this value be 4KB-aligned.
 * @return A void pointer to the allocated memory block, or NULL if no contiguous region of memory was found.
 */
void* kalloc(int size)
{
	acquire(&__kmemory_lock);

	int num_blocks = (size + sizeof(int) + KMEM_BLOCK_SIZE - 1) / KMEM_BLOCK_SIZE;
	int total_blocks = (KERNEL_MEMORY_END - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE;

	// Find a contiguous free region of memory
	int free_blocks = 0;
	for (int i = 0; i < total_blocks; i++) {

		/* look for continious memory */
		uint32_t index = KMEM_BITMAP_INDEX(KERNEL_MEMORY_START + i * KMEM_BLOCK_SIZE);
		uint32_t offset = KMEM_BITMAP_OFFSET(KERNEL_MEMORY_START + i * KMEM_BLOCK_SIZE);
		if (!(__kmemory_bitmap[index] & (1 << offset))) {
			free_blocks++;

			if (free_blocks == num_blocks) {
				// Mark the blocks as used in the bitmap
				for (int j = i - num_blocks + 1; j <= i; j++) {
					index = KMEM_BITMAP_INDEX(KERNEL_MEMORY_START + j * KMEM_BLOCK_SIZE);
					offset = KMEM_BITMAP_OFFSET(KERNEL_MEMORY_START + j * KMEM_BLOCK_SIZE);
					__kmemory_bitmap[index] |= (1 << offset);
				}

				// Write the size of the allocated block to the metadata block
				int* metadata = (int*) (KERNEL_MEMORY_START + (i - num_blocks + 1) * KMEM_BLOCK_SIZE);
				*metadata = num_blocks;

				// Return a pointer to the allocated memory block
				dbgprintf("[MEMORY] %s allocated %d blocks of data\n", current_running->name, num_blocks);
				__kmemory_used += num_blocks*KMEM_BLOCK_SIZE;
				current_running->kallocs++;

				release(&__kmemory_lock);
				return (void*)(KERNEL_MEMORY_START + (i - num_blocks + 1) * KMEM_BLOCK_SIZE + sizeof(int));
			}
		} else {
			/* if we found a block that is allocated, reset our search.  */
			free_blocks = 0;
		}
	}
	// No contiguous free region of memory was found
	release(&__kmemory_lock);
	return NULL;
}

/**
 * @brief Frees a previously allocated block of memory.
 *
 * This function releases a previously allocated block of memory for future use. It uses the metadata
 * block to determine the number of blocks to free, then clears the corresponding bits in the bitmap
 * to mark them as free. If the input pointer is NULL, the function simply returns without performing
 * any action.
 *
 * @param ptr A pointer to the start of the memory block to free.
 *
 * @return None.
 */
void kfree(void* ptr) {
	if (!ptr) {
		return;
	}
	
	acquire(&__kmemory_lock);

	// Calculate the index of the block in the memory region
	int block_index = (((uint32_t)ptr) - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE;

	// Read the size of the allocated block from the metadata block
	int* metadata = (int*) (KERNEL_MEMORY_START + block_index * KMEM_BLOCK_SIZE);
	int num_blocks = *metadata;
	dbgprintf("[MEMORY] %s freeing %d blocks of data\n", current_running->name, num_blocks);

	// Mark the blocks as free in the bitmap
	for (int i = 0; i < num_blocks; i++) {
		uint32_t index = KMEM_BITMAP_INDEX(KERNEL_MEMORY_START + (block_index + i) * KMEM_BLOCK_SIZE);
		uint32_t offset = KMEM_BITMAP_OFFSET(KERNEL_MEMORY_START + (block_index + i) * KMEM_BLOCK_SIZE);
		__kmemory_bitmap[index] &= ~(1 << offset);
	}

	release(&__kmemory_lock);
}

#define VMEM_HEAP 0xE0000000
void vmem_free_allocation(struct allocation* allocation)
{
	int num_pages = (allocation->size + PAGE_SIZE - 1) / PAGE_SIZE;
	for (int i = 0; i < num_pages; i++)
	{
		memory_free_page((void*) (VMEM_START_ADDRESS + (allocation->bits[i] * PAGE_SIZE)));
	}

	kfree(allocation->bits);
	kfree(allocation);
}

static int memory_process_used = 0;
#define MEMORY_PROCESS_SIZE 500*1024
void free(void* ptr)
{
	if((int)ptr == current_running->allocations->address){
		struct allocation* old = current_running->allocations;
		current_running->allocations = current_running->allocations->next;
		current_running->used_memory -= old->size;
		memory_process_used -= old->size;
		
		vmem_free_allocation(old);
		return;
	}

	struct allocation* iter = current_running->allocations;
	while(iter->next != NULL){
		if(iter->next->address == (int)ptr){
			
			struct allocation* save = iter->next;
			iter->next = iter->next->next;
			current_running->used_memory -= save->size;
			memory_process_used -= save->size;

			vmem_free_allocation(save);
			return;
		}
	}
}

int vmem_continious_allocation_map(struct allocation* allocation, uint32_t* address, int num)
{
	int permissions = PRESENT | READ_WRITE | USER;
	uint32_t* heap_table = vmem_get_page_table(VMEM_HEAP);
	for (int i = 0; i < num; i++)
		{
			/*
				1. Allocate a page
				2. Map it to virtual heap
				3. add it to bits
			*/
			uint32_t* paddr = memory_alloc_page();
			if(paddr == NULL){
				/* TODO: cleanup allocated pages */
				return -1;
			}
			int bit = ((uint32_t)paddr - VMEM_START_ADDRESS)/PAGE_SIZE;
			allocation->bits[i] = bit;
			table_set(heap_table, allocation->address+(i*PAGE_SIZE), paddr, permissions);
		}
}

void* malloc(int size)
{	
	/* For rewrite with pages. */
	int ret;
	int num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	struct allocation* allocation = kalloc(sizeof(struct allocation));
	allocation->bits = kalloc(sizeof(int)*num_pages);
	allocation->size = size;

	if(current_running->allocations == NULL){
		
		allocation->address = VMEM_HEAP;

		vmem_continious_allocation_map(allocation, allocation->address, num_pages);
		
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
			allocation->address = iter->address+iter->size;
			allocation->next = NULL;

			struct allocation* next = iter->next;
			iter->next = allocation;
			allocation->next = next;

			vmem_continious_allocation_map(allocation, allocation->address, num_pages);

			current_running->used_memory += size;
			memory_process_used += size;
			return (void*) allocation->address;
		}
		iter = iter->next;
	}

	allocation->address = iter->address+iter->size;
	vmem_continious_allocation_map(allocation, allocation->address, num_pages);
	allocation->next = NULL;

	iter->next = allocation;
	memory_process_used += size;
	return (void*) allocation->address;
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
#define VMEM_MANAGER_START 0x200000
#define VMEM_MANAGER_END 0x300000
#define VMEM_MANAGER_PAGES ((VMEM_MANAGER_END-VMEM_MANAGER_START) / PAGE_SIZE)
static bitmap_t __vmem_manager_bitmap;

static uint32_t* __vmem_manager_alloc()
{
	int bit = get_free_bitmap(__vmem_manager_bitmap, VMEM_MANAGER_PAGES);
	if(bit == -1)
		return NULL;

	uint32_t* paddr = (uint32_t*) (VMEM_MANAGER_START + (bit * PAGE_SIZE));
	memset(paddr, 0, PAGE_SIZE);

	dbgprintf("[VMEM MANAGER] Allocated page %d at 0x%x\n", bit, paddr);
	return paddr;
}

static void __vmem_manager_free(void* addr)
{
	if((uint32_t)addr > VMEM_MANAGER_END  ||  (uint32_t)addr < VMEM_MANAGER_START)
		return;

	int bit = (((uint32_t) addr) - VMEM_MANAGER_START) / PAGE_SIZE;
	if(bit < 0 || bit > (VMEM_MANAGER_PAGES))
		return;
	
	unset_bitmap(__vmem_manager_bitmap, bit);
	dbgprintf("VMEM MANAGER] Free page %d at 0x%x\n", bit, addr);
}

void mmap_driver_region(uint32_t addr, int size)
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
	dbgprintf("[Memory] Cleaning up pages from pcb.\n");

	uint32_t directory = (uint32_t)pcb->page_dir;
	uint32_t data_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(0x1000000)] & ~PAGE_MASK;

	int size = pcb->data_size;
	int i = 0;
	while(size > 4096){
		uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(0x1000000)] & ~PAGE_MASK))[TABLE_INDEX((0x1000000+(i*4096)))]& ~PAGE_MASK;
		memory_free_page((void*) data_page);
		size -= 4096;
		i++;
	}

	uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(0x1000000)] & ~PAGE_MASK))[TABLE_INDEX((0x1000000+(i*4096)))]& ~PAGE_MASK;
	memory_free_page((void*) data_page);

	uint32_t stack_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(0xEFFFFFF0)] & ~PAGE_MASK;
	uint32_t stack_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(0xEFFFFFF0)] & ~PAGE_MASK))[TABLE_INDEX(0xEFFFFFF0)]& ~PAGE_MASK;

	uint32_t heap_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX((VMEM_HEAP + MEMORY_PROCESS_SIZE*pcb->pid))] & ~PAGE_MASK;
	
	/* Free all malloc allocation */
	struct allocation* iter = pcb->allocations;
	while(iter != NULL)
	{
		struct allocation* old = iter;
		iter = iter->next;

		dbgprintf("[PCB] Cleaning up virtual allocation 0x%x\n", old->address);

		int num_pages = (old->size + PAGE_SIZE - 1) / PAGE_SIZE;
		for (int i = 0; i < num_pages; i++)
		{
			memory_free_page((void*) (VMEM_START_ADDRESS + (old->bits[i] * PAGE_SIZE)));
		}
	
		kfree(old->bits);
		kfree(old);
	}

	memory_free_page((void*) stack_page);
	memory_free_page((void*) stack_table);

	memory_free_page((void*) data_table);

	memory_free_page((void*) heap_table);
	__vmem_manager_free((void*) directory);
	dbgprintf("[Memory] Cleaning up pages from pcb [DONE].\n");
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
	uint32_t* process_directory = __vmem_manager_alloc();
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
	int start = VMEM_HEAP;
	uint32_t* process_heap_memory_table = memory_alloc_page();
	dbgprintf("[MALLOC] 0x%x set table for %s\n", process_heap_memory_table, pcb->name);
	//for (int addr = start; addr < start+MEMORY_PROCESS_SIZE; addr += PAGE_SIZE)
	//	table_set(process_heap_memory, addr, addr, permissions);

	dbgprintf("[INIT PROCESS] Mapped dynamic memory %x to %x\n", start, start);

	/* Insert page and data tables in directory. */
	directory_set(process_directory, start, process_heap_memory_table, permissions);
	directory_set(process_directory, 0xEFFFFFF0, process_stack_table, permissions);

	process_directory[0] = kernel_page_dir[0];
	//process_directory[1] = kernel_page_dir[1]; // HACK: FIX ME

	dbgprintf("[INIT PROCESS] Paging done.\n");
	pcb->page_dir = (uint32_t*)process_directory;
}

static void _kernel_memory_test()
{
    void* ptr = kalloc(1024);
    assert(ptr != NULL);
    memset(ptr, 0xAA, 1024);

    void* ptr2 = kalloc(2048);
    assert(ptr2 != NULL);

    memset(ptr2, 0xBB, 2048);

    kfree(ptr);

    void* ptr3 = kalloc(512);
    assert(ptr3 != NULL);
    memset(ptr3, 0xCC, 512);

    kfree(ptr2);

    void* ptr4 = kalloc(4096);
    assert(ptr4 != NULL);

    memset(ptr4, 0xDD, 4096);

    kfree(ptr3);
    kfree(ptr4);

	ptr = kalloc(1024);
    assert(ptr != NULL);

	memset(ptr, 0xAA, 1024);

    kfree(ptr);

    ptr2 = kalloc(1024);
    assert(ptr2 != NULL);
    assert(ptr2 == ptr);

    memset(ptr2, 0xBB, 1024);
    kfree(ptr2);
}


void init_paging()
{
	vmem.pages = create_bitmap(VMEM_TOTAL_PAGES);
	__vmem_manager_bitmap = create_bitmap(VMEM_MANAGER_PAGES);
	mutex_init(&vmem.lock);
	dbgprintf("[PAGIN] %d free pagable pages.\n", VMEM_TOTAL_PAGES);

	kernel_page_dir = __vmem_manager_alloc();
	uint32_t* kernel_page_table = memory_alloc_page();
	int permissions = PRESENT | READ_WRITE;
	for (int addr = 0; addr < 0x400000; addr += PAGE_SIZE)
		table_set(kernel_page_table, addr, addr, permissions);

	int start = VMEM_HEAP;
	uint32_t* kernel_heap_memory_table = memory_alloc_page();
	dbgprintf("[MALLOC] 0x%x set table for %s\n", kernel_heap_memory_table, "kernel");
	directory_set(kernel_page_dir, start, kernel_heap_memory_table, permissions);

	for (int i = 1; i < 7; i++)
	{
		uint32_t* kernel_page_table_memory = memory_alloc_page();
		for (int addr = 0x400000*i; addr < 0x400000*(i+1); addr += PAGE_SIZE)
			table_set(kernel_page_table_memory, addr, addr, permissions);
		dbgprintf("[MEMORY] Identity mapping 0x%x to 0x%x\n", 0x400000*i, 0x400000*(i+1));
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

void kernel_memory_init()
{
	mutex_init(&__kmemory_lock);
	//_kernel_memory_test();
}

/**
 * @brief Initializes all memory chunks and sets them to be free.
 * 
 * @return void
 */
void init_memory()
{
	kernel_memory_init();
}