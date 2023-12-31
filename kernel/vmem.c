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
#include <stdint.h>
#include <memory.h>
#include <serial.h>
#include <sync.h>
#include <bitmap.h>
#include <assert.h>

struct virtual_memory_allocator;

/* allocator prototypes */
static uint32_t* vmem_alloc(struct virtual_memory_allocator* vmem);
static void vmem_free(struct virtual_memory_allocator* vmem, void* addr);

uint32_t* kernel_page_dir = NULL;

static const int vmem_default_permissions = SUPERVISOR | PRESENT | READ_WRITE;
static const int vmem_user_permissions = USER | PRESENT | READ_WRITE;

static int VMEM_START_ADDRESS = 0;
static int VMEM_END_ADDRESS = 0;

static int VMEM_MANAGER_START = 0;
static int VMEM_MANAGER_END = 0;

#define VMEM_TOTAL_PAGES ((VMEM_END_ADDRESS-VMEM_START_ADDRESS) / PAGE_SIZE)

#define VMEM_MANAGER_PAGES ((VMEM_MANAGER_END-VMEM_MANAGER_START) / PAGE_SIZE)

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

static struct virtual_memory_allocator __vmem_default;
struct virtual_memory_allocator* vmem_default = &__vmem_default;

static struct virtual_memory_allocator __vmem_manager;
struct virtual_memory_allocator* vmem_manager = &__vmem_manager;

/* HELPER FUNCTIONS */
static inline uint32_t* vmem_get_page_table(struct pcb* pcb, uint32_t addr)
{
	return (uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(addr)] & ~PAGE_MASK);
}

static inline void vmem_map(uint32_t* page_table, uint32_t vaddr, uint32_t paddr, int access)
{
	page_table[TABLE_INDEX(vaddr)] = (paddr & ~PAGE_MASK) | (access == 0 ? vmem_default_permissions : vmem_user_permissions);
}

static inline void vmem_unmap(uint32_t* page_table, uint32_t vaddr)
{
	page_table[TABLE_INDEX(vaddr)] = 0;
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
		assert(bit != -1);

		paddr = (uint32_t*) (vmem->start + (bit * PAGE_SIZE));
		vmem->used_pages++;
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
		vmem->used_pages--;
		dbgprintf("VMEM MANAGER] Free page %d at 0x%x\n", bit, addr);

	});
}

static int vmem_page_align_size(int size)
{
	int multiple = PAGE_SIZE;
    int closest;
    int quotient = size / multiple;
    int remainder = size % multiple;

    if (remainder > 0) {
        closest = (quotient + 1) * multiple;
    } else {
        closest = quotient * multiple;
    }

	return closest;
}

static struct vmem_page_region* vmem_create_page_region(struct pcb* pcb, void* base, int num, int access)
{
	struct vmem_page_region* allocation = create(struct vmem_page_region);
	if(allocation == NULL){
		return NULL;
	}

	allocation->bits = kalloc(sizeof(int)*num);
	if(allocation->bits == NULL){
		kfree(allocation);
		return NULL;
	}

	allocation->refs = 0;
	allocation->size = num*PAGE_SIZE;
	allocation->used = 0;
	allocation->basevaddr = base;

	uint32_t* heap_table = vmem_get_page_table(pcb, VMEM_HEAP);
	for (int i = 0; i < num; i++){
		/*
		 * 1. Allocate a page
		 * 2. Map it to virtual heap
		 * 3. add it to bits
		 */
		uint32_t paddr = (uint32_t)vmem_default->ops->alloc(vmem_default);
		if(paddr == 0){
			kfree(allocation->bits);
			for (int j = 0; j < i; j++){
				vmem_default->ops->free(vmem_default, (void*) (VMEM_START_ADDRESS + (allocation->bits[j] * PAGE_SIZE)));
			}
			return NULL;
		}
		int bit = (paddr - VMEM_START_ADDRESS)/PAGE_SIZE;
		allocation->bits[i] = bit;
		//dbgprintf("Allocating %d continious blocks on heap 0x%x.\n", num, heap_table);
		vmem_map(heap_table, (uint32_t)allocation->basevaddr+(i*PAGE_SIZE), paddr, access);
	}

	return allocation;
}

static int vmem_page_region_alloc(struct vmem_page_region* pages, int size)
{
	ERR_ON_NULL(pages);

	pages->used += size;
	pages->refs++;

	return 0;
}

static int vmem_free_page_region(struct pcb* pcb, struct vmem_page_region* region, int size)
{
	ERR_ON_NULL(region);
	if(region->refs > 1){
		region->refs--;
		region->used -= size;
		return 0;
	}

	dbgprintf("Freeing %d pages. 0x%x\n", region->size/PAGE_SIZE, region->basevaddr);

	int num_pages = region->size / PAGE_SIZE;
	dbgprintf("Freeing %d pages\n", num_pages);
	for (int i = 0; i < num_pages; i++){
		if(region->bits[i] == 0) continue;
		
		/* Free the physical page */
		void* paddr = (void*) (vmem_default->start + (region->bits[i] * PAGE_SIZE));
		vmem_default->ops->free(vmem_default, (void*) paddr);

		/* Unmap the virtual page */
		void* vaddr = (void*) (region->basevaddr + (i * PAGE_SIZE));
		dbgprintf("Unmapping 0x%x\n", vaddr);
		uint32_t* heap_table = vmem_get_page_table(pcb, (uint32_t)vaddr);
		dbgprintf("Table: 0x%x\n", heap_table);

		vmem_unmap(heap_table, (uint32_t)vaddr);
	}
	kfree(region->bits);
	kfree(region);

	return 0;
}

int vmem_free_allocations(struct pcb* pcb)
{
	uint32_t heap_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(VMEM_HEAP)] & ~PAGE_MASK;
	assert(heap_table != 0);

	if(pcb->allocations->head == NULL){
		kfree(pcb->allocations);
		return 0;
	}
	ENTER_CRITICAL();
	
	/* Free all malloc allocation */
	struct allocation* iter = pcb->allocations->head;
	/**
	 * @brief Very weird bug: a corrupt allocation at 0x1b0004
	 * causes a page fault when trying to free it.
	 * This somehow fixes it, but I have no idea why.
	 */
	while(iter != NULL && iter != 0x1b0004){
		struct allocation* old = iter;
		iter = iter->next;

		dbgprintf("Freeing %d bytes of data from 0x%x (0x%x)\n", old->size, old->address, old);
		dbgprintf("head: 0x%x\n", pcb->allocations->head);
		vmem_free_page_region(pcb, old->region, old->size);
	
		kfree(old);
	}

	vmem_default->ops->free(vmem_default, (void*) heap_table);
	
	/* Free allocation list */
	kfree(pcb->allocations);

	LEAVE_CRITICAL();

	return 0;
}

/**
 * @brief Allocates a chunk of virtual memory for the specified process control block (PCB).
 * The vmem_continious_allocation_map() function is responsible for allocating a contiguous block of virtual memory for the given PCB.
 * @param pcb A pointer to the process control block (PCB) for which memory needs to be allocated.
 * @param allocation A pointer to the allocation structure that contains the allocation information.
 * @param address A pointer to the start of the allocated memory block.
 * @param num The number of pages to be allocated.
 * @param access The access permissions for the allocated pages.
 * @return int 0 if the allocation succeeds, or -1 if the allocation fails.
 */
int __deprecated vmem_continious_allocation_map(struct pcb* pcb, struct allocation* allocation, uint32_t* address, int num, int access)
{

	return -1;
}

/**
 * @brief Frees a chunk of virtual memory for the specified process control block (PCB).
 * 
 * @param allocation 
 */
void __deprecated vmem_free_allocation(struct allocation* allocation)
{

}


/**
 * @brief Frees a virtual stack allocation
 * 
 * @param pcb Process to free from.
 * @param ptr Pointer to the address to free.
 */
void vmem_stack_free(struct pcb* pcb, void* ptr)
{
	/* Check if allocation is first in list */

	if(pcb->allocations->head == NULL){
		warningf("Trying to free allocation from empty list.\n");
		return;
	}

	if(ptr == pcb->allocations->head->address){
		struct allocation* old = pcb->allocations->head;
		pcb->allocations->head = pcb->allocations->head->next;
		pcb->used_memory -= old->size;

		vmem_free_page_region(pcb, old->region, old->size);
		
		dbgprintf("[1] Free %d bytes of data from 0x%x\n", old->size, old->address);
		return;
	}

	/* Iterates over all allocations to find ptr */
	struct allocation* iter = pcb->allocations->head;
	while(iter->next != NULL){
		if(iter->next->address == ptr){
			
			struct allocation* save = iter->next;
			iter->next = iter->next->next;
			pcb->used_memory -= save->size;

			vmem_free_page_region(pcb, save->region, save->size);
			
			dbgprintf("[2] Free %d bytes of data from 0x%x\n", save->size, save->address);
			return;
		}
		iter = iter->next;
	}
}

/**
 * 
 * @brief Allocates a chunk of virtual memory for the specified process control block (PCB).
 * The vmem_stack_alloc() function is responsible for allocating a contiguous block of virtual memory for the given PCB.
 * It uses a page-aligned size and ensures that the allocation aligns with page boundaries for efficient memory management.
 * If the PCB's allocations list is empty, the function allocates memory from the beginning of the virtual memory heap.
 * Otherwise, it traverses the list of existing allocations to find a suitable gap for the new allocation. If a suitable space is found,
 * the function inserts the new allocation into the list and updates the memory map accordingly.
 * In case no suitable gap is found in the existing allocations, the function adds the new allocation at the end, extending the virtual memory heap.
 * It then updates the memory map to include the newly allocated pages.
 * @param pcb A pointer to the process control block (PCB) for which memory needs to be allocated.
 * @param _size The size of memory to be allocated in bytes.
 * @return A pointer to the start of the allocated memory block, or NULL if the allocation fails.
 * @note The function uses page-aligned sizes and ensures efficient memory utilization.
 * @note The function assumes that the vmem_continious_allocation_map() function is defined and handles memory mapping.
 * @note The function uses kalloc() to allocate memory for internal data structures (e.g., struct allocation and bits array).
 * TODO: Needs to be synchronized among threads.
 */
void* vmem_stack_alloc(struct pcb* pcb, int _size)
{
	int size = vmem_page_align_size(_size);
	int num_pages = size / PAGE_SIZE;

	struct allocation* allocation = create(struct allocation);
	if(allocation == NULL){
		warningf("Out memory\n");
		return NULL;
	}
	dbgprintf("New allocation: 0x%x\n", allocation);
	
	allocation->size = _size;
	allocation->used = _size;

	/**
	 * @brief Part1: Default case
	 * Create a physical page allocation and attach it to the virtual allocation.
	 * Setup the allocation size and adress and add it to the pcb.
	 */
	if(pcb->allocations->head == NULL){

		struct vmem_page_region* physical = vmem_create_page_region(pcb, (void*)VMEM_HEAP, num_pages, USER);
		if(physical == NULL){
			kfree(allocation);
			warningf("Out of heap memory\n");
			return NULL;
		}
		allocation->region = physical;
		
		vmem_page_region_alloc(physical, _size);

		allocation->address = (uint32_t*) VMEM_HEAP;
		allocation->size = _size;
		allocation->next = NULL;

		pcb->allocations->head = allocation;
		pcb->used_memory += size;

		dbgprintf("[1] Allocated %d bytes of data to 0x%x\n", _size, allocation->address);
		return (void*) allocation->address;
	}

	/**
	 * @brief Part 1.5: If first allocation is freed, allocate from start of heap.
	 */
	if(pcb->allocations->head->address > (uint32_t*) VMEM_HEAP && pcb->allocations->head->address <= (uint32_t*) VMEM_HEAP+size){

		/* TODO: Clean this up, redudent code */
		struct vmem_page_region* physical = vmem_create_page_region(pcb, (void*)VMEM_HEAP, num_pages, USER);
		if(physical == NULL){
			kfree(allocation);
			warningf("Out of heap memory\n");
			return NULL;
		}
		allocation->region = physical;
		
		vmem_page_region_alloc(physical, _size);

		allocation->address = (uint32_t*) VMEM_HEAP;
		allocation->size = _size;
		allocation->next = NULL;

		dbgprintf("[1.5] Allocated %d bytes of data to 0x%x\n", _size, allocation->address);
		return (void*) allocation->address;
	}

	/**
	 * @brief Part 2: Find spot for allocation.
	 * Iterate over all allocations and find a spot where the next allocation is far enough away.
	 * Two options, first option is finding a free allocation inside a already allocated phsyical page region.
	 * Then we can simply add new allocation to the exisitng one physical one.
	 * Second option is we need to allocate a new physical page allocation, between two existing ones.
	 */
	struct allocation* iter = pcb->allocations->head;
	while(iter->next != NULL){

		/**
		 * @brief This case checks if there is space inside a single physical allocation (between virtual allocations).
		 * Also, means the physical area is already mapped so we can attach to the existing one.
		 */
		if((uint32_t)(iter->next->address) - ((uint32_t)(iter->address)+iter->size) >= (uint32_t)_size && iter->region == iter->next->region){
			
			/* Found spot for allocation */
			allocation->address = (uint32_t*)((uint32_t)(iter->address)+iter->size);
			allocation->region = iter->region;

			struct allocation* next = iter->next;
			iter->next = allocation;
			allocation->next = next;

			vmem_page_region_alloc(iter->region, _size);
			dbgprintf("[2] Allocated %d bytes of data to 0x%x\n", _size, allocation->address);
			return (void*) allocation->address;
		}
	
		/**
		 * @brief This case checks if there is space at the end of a phsyical region.
		 * Only is possible if the next allocation is in a different physical region.
		 * @note This is mostly for reusing the virtual heap address space.
		 */
		int space_at_end = ((uint32_t)(iter->region->basevaddr)+iter->region->size) - ((uint32_t)(iter->address)+iter->size);
		if(iter->region != iter->next->region && space_at_end >= _size)
		{
			/* Found spot for allocation */
			allocation->address = (uint32_t*)((uint32_t)(iter->address)+iter->size);
			allocation->region = iter->region;

			struct allocation* next = iter->next;
			iter->next = allocation;
			allocation->next = next;

			vmem_page_region_alloc(iter->region, _size);

			dbgprintf("[2.5] Allocated %d bytes of data to 0x%x\n", _size, allocation->address);
			return (void*) allocation->address;

		}

		iter = iter->next;
	}

	/**
	 * @brief Part 2.5: Check if there is space at the end of the last allocation.
	 * This avoids creating new physical page allocations.
	 */
	int space_at_end = ((uint32_t)(iter->region->basevaddr)+iter->region->size) - ((uint32_t)(iter->address)+iter->size);
	if(space_at_end >= _size)
	{
		/* Found spot for allocation */
		allocation->address = (uint32_t*)((uint32_t)(iter->address)+iter->size);
		allocation->region = iter->region;

		struct allocation* next = iter->next;
		iter->next = allocation;
		allocation->next = next;

		vmem_page_region_alloc(iter->region, _size);

		dbgprintf("[3] Allocated %d bytes of data to 0x%x\n", _size, allocation->address);
		return (void*) allocation->address;
	}

	/**
	 * @brief Part 3: No spot found, allocate at end of heap.
	 * At this point we have not found a spot inbetween existing allocations, so append at the end.
	 * This means we need to allocate a new physical page allocation.
	 * @note "iter" will point to the last element in the allocation list.
	 */
	struct vmem_page_region* physical = vmem_create_page_region(pcb, (void*)((byte_t*)iter->region->basevaddr + iter->region->size), num_pages, USER);
	if(physical == NULL){
		kfree(allocation);
		warningf("Out of heap memory\n");
		return NULL;
	}
	allocation->region = physical;

	allocation->address = (uint32_t*)((uint32_t)(iter->region->basevaddr)+iter->region->size);
	vmem_page_region_alloc(allocation->region, _size);
	allocation->next = NULL;

	pcb->used_memory += size;
	iter->next = allocation;
	dbgprintf("[3.5] Allocated %d bytes of data to 0x%x\n", _size, allocation->address);
	return (void*) allocation->address;
}

void vmem_dump_heap(struct allocation* allocation)
{
	dbgprintf(" ------- Memory Heap --------\n");
	struct allocation* iter = allocation;
	struct vmem_page_region* region = NULL;
	while(iter != NULL){
		if(region != iter->region){
			region = iter->region;
			dbgprintf(" ------- Region 0x%x -> 0x%x (%d/%d) %d refs --------\n", region->basevaddr, region->basevaddr+region->size, region->used, region->size, region->refs);
		}
		dbgprintf("     0x%x --- size %d\n", iter->address, iter->used);
		iter = iter->next;
	}
	dbgprintf(" -------     &End     --------\n");
}

/**
 * @brief Initializes the virtual memory module.
 * The vmem_init() function is responsible for initializing the virtual memory module.
 * @param parent A pointer to the process control block (PCB) of the parent process.
 * @param thread A pointer to the process control block (PCB) of the thread.
 */
void vmem_init_process_thread(struct pcb* parent, struct pcb* thread)
{
	/**
	 * @brief Create the virtual memory for the thread.
	 * Mainly needs its own stack, but also needs to share
	 * the heap and data sections with the parent.
	 * @see https://github.com/joexbayer/RetrOS-32/issues/73
	 * TODO: How to handle heap? Share? need synchronization?
	 */
	
	/* inheret directory */
	uint32_t* thread_directory = vmem_default->ops->alloc(vmem_default);
	for (int i = 0; i < 1024; i++){
		/* copy over pages, this will include heap and data */
		if(parent->page_dir[i] != 0) thread_directory[i] = parent->page_dir[i];
	}

	/* Allocate table for stack */
	uint32_t* thread_stack_table = vmem_default->ops->alloc(vmem_default);
	
	/* create 8kb stack, 2 4kb pages */
	uint32_t* thread_stack_page = vmem_default->ops->alloc(vmem_default);
	memset(thread_stack_page, 0, PAGE_SIZE);
	vmem_map(thread_stack_table, VMEM_STACK, (uint32_t) thread_stack_page, USER);

	uint32_t* thread_stack_page2 = vmem_default->ops->alloc(vmem_default);
	memset(thread_stack_page2, 0, PAGE_SIZE);
	vmem_map(thread_stack_table, VMEM_STACK+PAGE_SIZE, (uint32_t) thread_stack_page2, USER);

	/* Insert and replace stack in directory. */
	vmem_add_table(thread_directory, VMEM_STACK, thread_stack_table, USER);

	thread->page_dir = (uint32_t*)thread_directory;
	thread->allocations = parent->allocations;

	dbgprintf("[INIT THREAD] Thread paging setup done.\n");
}

/**
 * @brief Initializes the virtual memory for the specified process control block (PCB).
 * The vmem_init_process() function is responsible for initializing the virtual memory for the given PCB.
 * @param pcb A pointer to the process control block (PCB) for which memory needs to be initialized.
 * @param data in memory data to be copied into the process data section.
 * @param size size of the data to be copied.
 * TODO: Should probably be on demand paging.
 */
void vmem_init_process(struct pcb* pcb, byte_t* data, int size)
{
	int allocated_pages = 0;

	/* Allocate directory and tables for data and stack */
	uint32_t* process_directory = vmem_default->ops->alloc(vmem_default);
	allocated_pages++;
	uint32_t* process_data_table = vmem_default->ops->alloc(vmem_default);
	allocated_pages++;
	uint32_t* process_stack_table = vmem_default->ops->alloc(vmem_default);
	allocated_pages++;
	uint32_t* process_heap_table = vmem_default->ops->alloc(vmem_default);
	allocated_pages++;

	dbgprintf("[INIT PROCESS] Directory: 0x%x\n", process_directory);
	dbgprintf("[INIT PROCESS] Data: 	 0x%x\n", process_data_table);
	dbgprintf("[INIT PROCESS] Stack:	 0x%x\n", process_stack_table);
	dbgprintf("[INIT PROCESS] Heap: 	 0x%x\n", process_heap_table);

	/* Any process should have the kernel first 4mb mapped */
	for (int i = 0; i < 1024; i++){
		if(kernel_page_dir[i] != 0) process_directory[i] = kernel_page_dir[i];
	}

	uint32_t* process_data_page;
	/* Map the process data to a page */
	int i = 0;
	while (size > PAGE_SIZE){
		process_data_page = vmem_default->ops->alloc(vmem_default);
		allocated_pages++;
		/* copy in process data. */
		memcpy(process_data_page, &data[i*PAGE_SIZE], PAGE_SIZE);
		vmem_map(process_data_table, VMEM_DATA+(i*PAGE_SIZE), (uint32_t) process_data_page, USER);
		size -= PAGE_SIZE;
		i++;
	}
	/* fill in rest. */
	process_data_page = vmem_default->ops->alloc(vmem_default);
	allocated_pages++;
	memcpy(process_data_page, &data[i*PAGE_SIZE], size);
	vmem_map(process_data_table, VMEM_DATA+(i*PAGE_SIZE), (uint32_t) process_data_page, USER);
	dbgprintf("[INIT PROCESS] Finished mapping data.\n");

	/* Map the process stack 8Kb to a page */
	uint32_t* process_stack_page = vmem_default->ops->alloc(vmem_default);
	allocated_pages++;
	memset(process_stack_page, 0, PAGE_SIZE);
	dbgprintf("[INIT PROCESS] Finished allocating 1/2 stack page.\n");
	vmem_map(process_stack_table, VMEM_STACK, (uint32_t) process_stack_page, USER);
	dbgprintf("[INIT PROCESS] Finished mapping 1/2 stack page.\n");

	uint32_t* process_stack_page2 = vmem_default->ops->alloc(vmem_default);
	allocated_pages++;
	dbgprintf("[INIT PROCESS] Finished allocating 2/2 stack page.\n");
	memset(process_stack_page2, 0, PAGE_SIZE);
	dbgprintf("[INIT PROCESS] Finished clearing 2/2 stack page.\n");
	vmem_map(process_stack_table, VMEM_STACK-PAGE_SIZE, (uint32_t) process_stack_page2, USER);
	dbgprintf("[INIT PROCESS] Finished mapping stack.\n");

	/* Insert page and data tables in directory. */
	vmem_add_table(process_directory, VMEM_HEAP, process_heap_table, USER);
	vmem_add_table(process_directory, VMEM_STACK, process_stack_table, USER);
	vmem_add_table(process_directory, VMEM_DATA, process_data_table, USER); 

	pcb->allocations = create(struct virtual_allocations);
	if(pcb->allocations == NULL){
		kernel_panic("Out of memory while allocating virtual memory allocations.");
	}
	pcb->allocations->head = NULL;
	pcb->allocations->spinlock = 0;


	dbgprintf("[INIT PROCESS] Process paging setup done: allocated %d pages.\n", allocated_pages);
	pcb->page_dir = (uint32_t*)process_directory;
}

/**
 * @brief Cleans up the virtual memory for the specified process control block (PCB).
 * Responsible for cleaning up the virtual stack for the given PCB. 
 * Does not cleanup the heap or data sections.
 * @param thread A pointer to the process control block (PCB) for which memory needs to be cleaned up.
 */
void vmem_cleanup_process_thead(struct pcb* thread)
{
	/**
	 * @brief Free the stack for the thread.
	 * @see https://github.com/joexbayer/RetrOS-32/issues/73
	 * TODO: Data sections should remain until the original process is killed.
	 * 
	 */

	uint32_t stack_table = (uint32_t)thread->page_dir[DIRECTORY_INDEX(VMEM_STACK)] & ~PAGE_MASK;
	uint32_t stack_page = (uint32_t)((uint32_t*)(thread->page_dir[DIRECTORY_INDEX(VMEM_STACK)] & ~PAGE_MASK))[TABLE_INDEX(VMEM_STACK)]& ~PAGE_MASK;
	uint32_t stack_page2 = (uint32_t)((uint32_t*)(thread->page_dir[DIRECTORY_INDEX((VMEM_STACK-PAGE_SIZE))] & ~PAGE_MASK))[TABLE_INDEX(VMEM_STACK)]& ~PAGE_MASK;

	vmem_default->ops->free(vmem_default, (void*) stack_page);
	vmem_default->ops->free(vmem_default, (void*) stack_page2);
	vmem_default->ops->free(vmem_default, (void*) stack_table);

	vmem_default->ops->free(vmem_default, (void*) thread->page_dir);

}

/**
 * @brief Frees all virtual memory pages allocated for the specified process control block (PCB).
 * The vmem_cleanup_process() function is responsible for freeing all virtual memory pages allocated for the given PCB.
 * It first frees all data pages, then the stack pages, and finally the heap pages.
 * @param pcb A pointer to the process control block (PCB) for which memory needs to be freed.
 * @return void
 * @note The function uses the vmem_default_ops structure to free the virtual memory pages.
 */
void vmem_cleanup_process(struct pcb* pcb)
{
	/**
	 * @brief A process can not be "cleaned" unless all of its threads are dead.
	 * @see https://github.com/joexbayer/RetrOS-32/issues/84
	 * 
	 * Should proably be done before this function is entered.
	 * Theoretically a process's threads, could have threads, like a tree.
	 * TODO: Find a solution...
	 */

	int freed_pages = 0;

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
		uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(VMEM_DATA)] & ~PAGE_MASK))[TABLE_INDEX((VMEM_DATA+(i*4096)))]& ~PAGE_MASK;
		vmem_default->ops->free(vmem_default, (void*) data_page);
		freed_pages++;
		size -= 4096;
		i++;
	}
	uint32_t data_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(VMEM_DATA)] & ~PAGE_MASK))[TABLE_INDEX((VMEM_DATA+(i*4096)))]& ~PAGE_MASK;
	
	vmem_default->ops->free(vmem_default, (void*) data_page);
	freed_pages++;
	vmem_default->ops->free(vmem_default, (void*) data_table);
	freed_pages++;

	dbgprintf("[Memory] Cleaning up data from pcb [DONE].\n");

	/**
	 * Free all stack pages
	 */
	uint32_t stack_table = (uint32_t)pcb->page_dir[DIRECTORY_INDEX(VMEM_STACK)] & ~PAGE_MASK;
	uint32_t stack_page = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(VMEM_STACK)] & ~PAGE_MASK))[TABLE_INDEX(VMEM_STACK)]& ~PAGE_MASK;
	uint32_t stack_page2 = (uint32_t)((uint32_t*)(pcb->page_dir[DIRECTORY_INDEX((VMEM_STACK-PAGE_SIZE))] & ~PAGE_MASK))[TABLE_INDEX(VMEM_STACK)]& ~PAGE_MASK;

	vmem_default->ops->free(vmem_default, (void*) stack_page);
	freed_pages++;
	vmem_default->ops->free(vmem_default, (void*) stack_page2);
	freed_pages++;
	vmem_default->ops->free(vmem_default, (void*) stack_table);
	freed_pages++;

	dbgprintf("[Memory] Cleaning up stack from pcb [DONE].\n");

	/**
	 * Free all heap allocated memory.
	 */
	vmem_free_allocations(pcb);

	dbgprintf("[Memory] Cleaning up allocations from pcb [DONE].\n");

	/**
	 * Lastly free directory.
	 */
	vmem_default->ops->free(vmem_default, (void*) directory);
	freed_pages++;
	dbgprintf("[Memory] Cleaning up pages from pcb: freed %d pages.\n", freed_pages);
}

/**
 * @brief Initializes the virtual memory for the kernel.
 * The vmem_init_kernel() function is responsible for initializing the virtual memory for the kernel.
 * It first allocates a page directory and a page table for the kernel.
 * It then maps the first 4MB of the virtual memory to the first 4MB of the physical memory.
 * It then maps the virtual memory heap to the virtual memory address space.
 * @return void
 * @note The function uses the vmem_default_ops structure to initialize the virtual memory allocator for the kernel.
 */
void vmem_init_kernel()
{	
	int total_mem = memory_map_get()->total;

	kernel_page_dir = vmem_manager->ops->alloc(vmem_manager);

	/* identity map first 4 mb of data. */
	uint32_t* kernel_page_table = vmem_manager->ops->alloc(vmem_manager);
	for (int addr = 0; addr < 0x400000; addr += PAGE_SIZE){
		vmem_map(kernel_page_table, addr, addr, SUPERVISOR);
	}

	int start = VMEM_HEAP;
	uint32_t* kernel_heap_memory_table = vmem_manager->ops->alloc(vmem_manager);;
	vmem_add_table(kernel_page_dir, start, kernel_heap_memory_table, SUPERVISOR);

	/* identity map rest of memory above 4MB */
	dbgprintf("Initiating memory from 0x%x - %d\n", 0x400000, ((total_mem)/(1024*1024)) - 4);
	for (int i = 1; i < 16/4; i++){
		uint32_t* kernel_page_table_memory = vmem_manager->ops->alloc(vmem_manager);
		if (kernel_page_table_memory == NULL){
			PANIC();
		}
		
		for (int k = 0; k < 1024; k += 1){
			int addr = 0x400000*i + k*PAGE_SIZE;
			vmem_map(kernel_page_table_memory, addr, addr, SUPERVISOR);
		}
		vmem_add_table(kernel_page_dir, 0x400000*i, kernel_page_table_memory, SUPERVISOR);
		dbgprintf("Initiated memory between 0x%x and 0x%x\n", 0x400000*i, 0x400000*(i+1));
	}
	dbgprintf("Initiated memory between 0x%x and 0x%x\n", 0x400000, 0x400000 + total_mem - 4*1024*1024);
	
	/* test if 0x80d000 is identity mapped */

	vmem_add_table(kernel_page_dir, 0, kernel_page_table, SUPERVISOR);
	
	dbgprintf("[INIT KERNEL] Directory: 		0x%x\n", kernel_page_dir);
	dbgprintf("[INIT KERNEL] 0x0 - 0x400000: 	0x%x\n", kernel_page_table);
	dbgprintf("[INIT KERNEL] Heap (Kthreads): 	0x%x\n", kernel_heap_memory_table);
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
	uint32_t* kernel_page_table_driver = vmem_default->ops->alloc(vmem_default);;
	for (int i = 0; i < size; i++)
		vmem_map(kernel_page_table_driver, (uint32_t) addr+(PAGE_SIZE*i), (uint32_t) addr+(PAGE_SIZE*i), SUPERVISOR);
	
	dbgprintf("[mmap] Page for 0x%x set\n", addr);

	vmem_add_table(kernel_page_dir,  addr, kernel_page_table_driver, SUPERVISOR);
	return;
}

int vmem_total_usage()
{
	int used_pages = vmem_default->used_pages + vmem_manager->used_pages;

	return used_pages * PAGE_SIZE;
}

/**
 * @brief Initializes the virtual memory module.
 * The vmem_init() function is responsible for initializing the virtual memory module.
 * It first initializes the virtual memory allocator for the kernel and then initializes the virtual memory allocator for the process manager.
 * @return void
 * @note The function uses kalloc() to allocate memory for the virtual memory allocators.
 * @note The function uses the vmem_default_ops structure to initialize the virtual memory allocator for the kernel.
 * @note The function uses the vmem_manager_ops structure to initialize the virtual memory allocator for the process manager.
 */
void vmem_init()
{
	/* first 1Mb is for virtual memory managment, this gives us 256 management pages */
	VMEM_MANAGER_START 	= memory_map_get()->virtual.from;
	VMEM_MANAGER_END 	= memory_map_get()->virtual.from + MB(1);

	VMEM_START_ADDRESS 	= memory_map_get()->virtual.from + MB(1);
	VMEM_END_ADDRESS 	= memory_map_get()->virtual.to;

	vmem_allocator_create(vmem_default, VMEM_START_ADDRESS, VMEM_END_ADDRESS);
	dbgprintf("Manager start: 0x%x - 0x%x (%d)\n", VMEM_MANAGER_START, VMEM_MANAGER_END, VMEM_MANAGER_PAGES);

	vmem_allocator_create(vmem_manager, VMEM_MANAGER_START, VMEM_MANAGER_END);
	dbgprintf("Default: 0x%x - 0x%x (%d)\n", VMEM_START_ADDRESS, VMEM_END_ADDRESS, VMEM_TOTAL_PAGES);

	dbgprintf("[VIRTUAL MEMORY] %d free pagable pages.\n", VMEM_TOTAL_PAGES);
	dbgprintf("[VIRTUAL MEMORY] %d free pagable management pages.\n", VMEM_MANAGER_PAGES);
}
