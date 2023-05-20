/**
 * @file vmem.c
 * @author Joe Bayer (joexbayer)
 * @brief Virtual memory module
 * @version 0.1
 * @date 2023-03-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <memory.h>
#include <serial.h>
#include <sync.h>
#include <bitmap.h>
#include <assert.h>
#include <vbe.h>
struct virtual_memory_allocator;

static uint32_t* vmem_alloc(struct virtual_memory_allocator* vmem);
static void vmem_free(struct virtual_memory_allocator* vmem, void* addr);

uint32_t* kernel_page_dir = NULL;

static int vmem_default_permissions = SUPERVISOR | PRESENT | READ_WRITE;
static int vmem_user_permissions = USER | PRESENT | READ_WRITE;

struct virtual_memory_operations {
	uint32_t* (*alloc)(struct virtual_memory_allocator* vmem);
	void (*free)(struct virtual_memory_allocator* vmem, void* page);
} vmem_default_ops = {
	.alloc = &vmem_alloc,
	.free = &vmem_free
};

struct virtual_memory_allocator {
	int used_pages;
	int total_pages;
	bitmap_t pages;

	uint32_t start;
	uint32_t end;

	struct virtual_memory_operations* ops;
	mutex_t lock;
};

struct virtual_memory_allocator __vmem_default;
struct virtual_memory_allocator* vmem_default = &__vmem_default;

struct virtual_memory_allocator __vmem_manager;
struct virtual_memory_allocator* vmem_manager = &__vmem_manager;

/* HELPER FUNCTIONS */
static inline uint32_t* vmem_get_page_table(uint32_t addr)
{
	return (uint32_t*)(current_running->page_dir[DIRECTORY_INDEX(addr)] & ~PAGE_MASK);
}

static inline void vmem_map(uint32_t* page_table, uint32_t vaddr, uint32_t paddr, int access)
{
	page_table[TABLE_INDEX(vaddr)] = (paddr & ~PAGE_MASK) | (access == 0 ? vmem_default_permissions : vmem_user_permissions);
}

static inline void vmem_add_table(uint32_t* directory, uint32_t vaddr, uint32_t* table, int access)
{
	directory[DIRECTORY_INDEX(vaddr)] = (((uint32_t) table) & ~PAGE_MASK) | (access == 0 ? vmem_default_permissions : vmem_user_permissions);
}

/**
 * Allocates a page of virtual memory from the given virtual memory allocator.
 * @param struct virtual_memory_allocator* vmem: pointer to virtual memory allocator
 * @return pointer to the allocated page or NULL if no free pages are available.
 */
static uint32_t* vmem_alloc(struct virtual_memory_allocator* vmem)
{
	uint32_t* paddr = NULL;
	
	LOCK(vmem, {

		int bit = get_free_bitmap(vmem->pages, vmem->total_pages);
		if(bit == -1){
			break;
		}
			
		paddr = (uint32_t*) (vmem->start + (bit * PAGE_SIZE));
		//memset(paddr, 0, PAGE_SIZE);

		//dbgprintf("[VMEM MANAGER] Allocated page %d at 0x%x\n", bit, paddr);

	});

	return paddr;
}

/**
 * Frees the virtual memory page at the given address in the given virtual memory allocator.
 * @param struct virtual_memory_allocator* vmem: pointer to virtual memory allocator
 * @param void* addr: pointer to the address of the virtual memory page to be freed
 * @return void 
 */
static void vmem_free(struct virtual_memory_allocator* vmem, void* addr)
{
	LOCK(vmem, {

		if((uint32_t)addr > vmem->end  ||  (uint32_t)addr < vmem->start)
			break;

		int bit = (((uint32_t) addr) - vmem->start) / PAGE_SIZE;
		if(bit < 0 || bit > (vmem->total_pages))
			break;
		
		unset_bitmap(vmem->pages, bit);
		//dbgprintf("VMEM MANAGER] Free page %d at 0x%x\n", bit, addr);

	});
}

int vmem_continious_allocation_map(struct allocation* allocation, uint32_t* address, int num, int access)
{

	uint32_t* heap_table = vmem_get_page_table(VMEM_HEAP);
	for (int i = 0; i < num; i++){
		/*
			1. Allocate a page
			2. Map it to virtual heap
			3. add it to bits
		*/
		uint32_t paddr = (uint32_t)vmem_default->ops->alloc(vmem_default);
		if(paddr == 0){
			/* TODO: cleanup allocated pages */
			return -1;
		}
		int bit = (paddr - VMEM_START_ADDRESS)/PAGE_SIZE;
		allocation->bits[i] = bit;
		//dbgprintf("Allocating %d continious blocks on heap 0x%x.\n", num, heap_table);
		vmem_map(heap_table, (uint32_t)allocation->address+(i*PAGE_SIZE), paddr, access);
	}


	return 0;
}


void vmem_free_allocation(struct allocation* allocation)
{
	int num_pages = (allocation->size + PAGE_SIZE - 1) / PAGE_SIZE;
	for (int i = 0; i < num_pages; i++){
		vmem_default->ops->free(vmem_default, (void*) (VMEM_START_ADDRESS + (allocation->bits[i] * PAGE_SIZE)));
	}

	kfree(allocation->bits);
	kfree(allocation);
}

void vmem_init_process(struct pcb* pcb, char* data, int size)
{

	/* Allocate directory and tables for data and stack */
	uint32_t* process_directory = vmem_manager->ops->alloc(vmem_manager);
	dbgprintf("[INIT PROCESS] Directory: 0x%x\n", process_directory);
	uint32_t* process_data_table = vmem_manager->ops->alloc(vmem_manager);
	dbgprintf("[INIT PROCESS] Data: 	 0x%x\n", process_data_table);
	uint32_t* process_stack_table = vmem_manager->ops->alloc(vmem_manager);
	dbgprintf("[INIT PROCESS] Stack:	 0x%x\n", process_stack_table);
	uint32_t* process_heap_table = vmem_manager->ops->alloc(vmem_manager);
	dbgprintf("[INIT PROCESS] Heap: 	 0x%x\n", process_heap_table);

	/* Any process should have the kernel first 4mb mapped */
	for (int i = 0; i < 1024; i++){
		if(kernel_page_dir[i] != 0) process_directory[i] = kernel_page_dir[i];
	}

	/* Map the process data to a page */
	int i = 0;
	while (size > PAGE_SIZE){
		uint32_t* process_data_page = vmem_default->ops->alloc(vmem_default);;
		memcpy(process_data_page, &data[i*PAGE_SIZE], PAGE_SIZE);
		vmem_map(process_data_table, VMEM_DATA+(i*PAGE_SIZE), (uint32_t) process_data_page, USER);
		size -= PAGE_SIZE;
		i++;
	}
	uint32_t* process_data_page = vmem_default->ops->alloc(vmem_default);;
	memcpy(process_data_page, &data[i*PAGE_SIZE], size);
	vmem_map(process_data_table, VMEM_DATA+(i*PAGE_SIZE), (uint32_t) process_data_page, USER);
	dbgprintf("[INIT PROCESS] Finished mapping data.\n");

	/* Map the process stack to a page */
	uint32_t* process_stack_page = vmem_default->ops->alloc(vmem_default);;
	memset(process_stack_page, 0, PAGE_SIZE);
	vmem_map(process_stack_table, VMEM_STACK, (uint32_t) process_stack_page, USER);
	dbgprintf("[INIT PROCESS] Finished mapping stack.\n");

	/* Insert page and data tables in directory. */
	vmem_add_table(process_directory, VMEM_HEAP, process_heap_table, USER);
	vmem_add_table(process_directory, VMEM_STACK, process_stack_table, USER);
	vmem_add_table(process_directory, VMEM_DATA, process_data_table, USER); 

	dbgprintf("[INIT PROCESS] Process paging setup done.\n");
	pcb->page_dir = (uint32_t*)process_directory;
}

void vmem_cleanup_process(struct pcb* pcb)
{
	dbgprintf("[Memory] Cleaning up pages from pcb.\n");

	uint32_t directory = (uint32_t)pcb->page_dir;

	/**
	 * Free all data pages, first get correct data table then
	 * free all pages based on the size of pcb.
	 */
	uint32_t data_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(VMEM_DATA)] & ~PAGE_MASK;
	assert(data_table != 0);

	int size = pcb->data_size;
	int i = 0;
	while(size > 4096){
		uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(VMEM_DATA)] & ~PAGE_MASK))[TABLE_INDEX((0x1000000+(i*4096)))]& ~PAGE_MASK;
		vmem_default->ops->free(vmem_default, (void*) data_page);
		size -= 4096;
		i++;
	}
	uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(VMEM_DATA)] & ~PAGE_MASK))[TABLE_INDEX((0x1000000+(i*4096)))]& ~PAGE_MASK;
	
	vmem_default->ops->free(vmem_default, (void*) data_page);
	vmem_manager->ops->free(vmem_default, (void*) data_table);

	/**
	 * Free all stack pages (currently only 1)
	 */
	uint32_t stack_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(VMEM_STACK)] & ~PAGE_MASK;
	uint32_t stack_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(VMEM_STACK)] & ~PAGE_MASK))[TABLE_INDEX(0xEFFFFFF0)]& ~PAGE_MASK;
	
	vmem_default->ops->free(vmem_default, (void*) stack_page);
	vmem_manager->ops->free(vmem_default, (void*) stack_table);

	/**
	 * Free all heap allocated memory.
	 */
	uint32_t heap_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(VMEM_HEAP)] & ~PAGE_MASK;
	
	/* Free all malloc allocation */
	struct allocation* iter = pcb->allocations;
	while(iter != NULL){
		struct allocation* old = iter;
		iter = iter->next;

		dbgprintf("[PCB] Cleaning up virtual allocation 0x%x\n", old->address);

		int num_pages = (old->size + PAGE_SIZE - 1) / PAGE_SIZE;
		for (int i = 0; i < num_pages; i++)
		{
			//dbgprintf("[PCB] Cleaning up continious virtual allocation 0x%x\n",  old->address);
			vmem_default->ops->free(vmem_default, (void*) (VMEM_START_ADDRESS + (old->bits[i] * PAGE_SIZE)));
		}
	
		kfree(old->bits);
		kfree(old);
	}
	vmem_manager->ops->free(vmem_default, (void*) heap_table);
	
	/**
	 * Lastly free directory.
	 */
	vmem_manager->ops->free(vmem_default, (void*) directory);
	dbgprintf("[Memory] Cleaning up pages from pcb [DONE].\n");
}

void vmem_init_kernel()
{	
	kernel_page_dir = vmem_manager->ops->alloc(vmem_manager);
	dbgprintf("[INIT KERNEL] Directory: 		0x%x\n", kernel_page_dir);

	uint32_t* kernel_page_table = vmem_default->ops->alloc(vmem_default);
	for (int addr = 0; addr < 0x400000; addr += PAGE_SIZE){
		vmem_map(kernel_page_table, addr, addr, SUPERVISOR);
	}
	dbgprintf("[INIT KERNEL] 0x0 - 0x400000: 	0x%x\n", kernel_page_table);

	int start = VMEM_HEAP;
	uint32_t* kernel_heap_memory_table = vmem_default->ops->alloc(vmem_default);;
	vmem_add_table(kernel_page_dir, start, kernel_heap_memory_table, SUPERVISOR);
	dbgprintf("[INIT KERNEL] Heap (Kthreads): 	0x%x\n", kernel_heap_memory_table);

	for (int i = 1; i < 7; i++){
		uint32_t* kernel_page_table_memory = vmem_default->ops->alloc(vmem_default);;
		for (int addr = 0x400000*i; addr < 0x400000*(i+1); addr += PAGE_SIZE)
			vmem_map(kernel_page_table_memory, addr, addr, SUPERVISOR);
		vmem_add_table(kernel_page_dir, 0x400000*i, kernel_page_table_memory, SUPERVISOR);
	}
	
	/**
	 * Identity map vesa color framebuffer
	 * TODO: Use vmem_map_driver_region
	 */
	uint32_t* kernel_page_table_vesa = vmem_default->ops->alloc(vmem_default);;
	for (int addr = 0; addr < (vbe_info->width*vbe_info->height*(vbe_info->bpp/8))+1; addr += PAGE_SIZE)
		vmem_map(kernel_page_table_vesa, vbe_info->framebuffer+addr, vbe_info->framebuffer+addr, SUPERVISOR);

	vmem_add_table(kernel_page_dir, 0, kernel_page_table, SUPERVISOR);

	vmem_add_table(kernel_page_dir, vbe_info->framebuffer, kernel_page_table_vesa, SUPERVISOR); 
}

int vmem_allocator_create(struct virtual_memory_allocator* allocator, int from, int to)
{
	allocator->start = from;
	allocator->end = to;
	allocator->total_pages = (to-from)/PAGE_SIZE;
	allocator->ops = &vmem_default_ops;
	allocator->used_pages = 0;
	allocator->pages = create_bitmap(allocator->total_pages);
	mutex_init(&allocator->lock);
	dbgprintf("Created new allocator\n");


	return 0;
}

void vmem_map_driver_region(uint32_t addr, int size)
{
	uint32_t* kernel_page_table_e1000 = vmem_default->ops->alloc(vmem_default);;
	for (int i = 0; i < size; i++)
		vmem_map(kernel_page_table_e1000, (uint32_t) addr+(PAGE_SIZE*i), (uint32_t) addr+(PAGE_SIZE*i), SUPERVISOR);
	
	dbgprintf("[mmap] Page for 0x%x set\n", addr);

	vmem_add_table(kernel_page_dir,  addr, kernel_page_table_e1000, SUPERVISOR);
	return;
}

void vmem_init()
{
	vmem_allocator_create(vmem_default, VMEM_START_ADDRESS, VMEM_MAX_ADDRESS);
	vmem_allocator_create(vmem_manager, VMEM_MANAGER_START, VMEM_MANAGER_END);

	dbgprintf("[VIRTUAL MEMORY] %d free pagable pages.\n", VMEM_TOTAL_PAGES);
	dbgprintf("[VIRTUAL MEMORY] %d free pagable management pages.\n", VMEM_MANAGER_PAGES);
}