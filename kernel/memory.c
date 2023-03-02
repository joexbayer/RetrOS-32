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


/* Dynamic kernel memory */
#define BLOCK_SIZE 512
#define BLOCKS_PER_BYTE 8
#define BITMAP_INDEX(addr) ((addr - KERNEL_MEMORY_START) / BLOCK_SIZE / BLOCKS_PER_BYTE)
#define BITMAP_OFFSET(addr) ((addr - KERNEL_MEMORY_START) / BLOCK_SIZE % BLOCKS_PER_BYTE)

/* need to add static bitmap as, bitmap_t uses kalloc */
static uint8_t __kmemory_bitmap[(KERNEL_MEMORY_END - KERNEL_MEMORY_START) / BLOCK_SIZE / BLOCKS_PER_BYTE];
static mutex_t __kmemory_lock;

void kernel_memory_init()
{
	mutex_init(&__kmemory_lock);
}

/**
 * @brief Allocates sequential chunks with fixed size 4Kb each.
 * Will allocate multiple chunks if needed.
 * 
 * @param int size, how much memory is needed (Best if 4Kb aligned.).
 * @return void* to memory location. NULL if not enough continious chunks.
 */
void* kalloc(int size)
{
	acquire(&__kmemory_lock);

    int num_blocks = (size + sizeof(int) + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Find a contiguous free region of memory
    int free_blocks = 0;
    for (int i = 0; i < (KERNEL_MEMORY_END - KERNEL_MEMORY_START) / BLOCK_SIZE; i++) {

		/* look for continious memory */
        if (!(__kmemory_bitmap[BITMAP_INDEX(KERNEL_MEMORY_START + i * BLOCK_SIZE)] & (1 << BITMAP_OFFSET(KERNEL_MEMORY_START + i * BLOCK_SIZE)))) {
            free_blocks++;

            if (free_blocks == num_blocks) {
                // Mark the blocks as used in the bitmap
                for (int j = i - num_blocks + 1; j <= i; j++) {
                    __kmemory_bitmap[BITMAP_INDEX(KERNEL_MEMORY_START + j * BLOCK_SIZE)] |= (1 << BITMAP_OFFSET(KERNEL_MEMORY_START + j * BLOCK_SIZE));
                }

                // Write the size of the allocated block to the metadata block
                int* metadata = (int*) (KERNEL_MEMORY_START + (i - num_blocks + 1) * BLOCK_SIZE);
                *metadata = num_blocks;

                // Return a pointer to the allocated memory block
				dbgprintf("[MEMORY] Allocated %d blocks of data\n", num_blocks);
				release(&__kmemory_lock);
                return (void*)KERNEL_MEMORY_START + (i - num_blocks + 1) * BLOCK_SIZE + sizeof(int);
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

void kfree(void* ptr) {
    if (!ptr) {
        return;
    }
	
	acquire(&__kmemory_lock);

    // Calculate the index of the block in the memory region
    int block_index = (((uint32_t)ptr) - KERNEL_MEMORY_START) / BLOCK_SIZE;

    // Read the size of the allocated block from the metadata block
    int* metadata = (int*) (KERNEL_MEMORY_START + block_index * BLOCK_SIZE);
    int num_blocks = *metadata;
	dbgprintf("[MEMORY] freeing %d blocks of data\n", num_blocks);

    // Mark the blocks as free in the bitmap
    for (int i = 0; i < num_blocks; i++) {
        __kmemory_bitmap[BITMAP_INDEX(KERNEL_MEMORY_START + (block_index + i) * BLOCK_SIZE)] &= ~(1 << BITMAP_OFFSET(KERNEL_MEMORY_START + (block_index + i) * BLOCK_SIZE));
    }

	release(&__kmemory_lock);
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


static int memory_process_used = 0;
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
		uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(0x1000000)] & ~PAGE_MASK))[TABLE_INDEX((0x1000000+(i*4096)))]& ~PAGE_MASK;
		memory_free_page((void*) data_page);
		size -= 4096;
		i++;
	}

	uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(0x1000000)] & ~PAGE_MASK))[TABLE_INDEX((0x1000000+(i*4096)))]& ~PAGE_MASK;
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

	memory_free_page(pcb->page_dir);
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