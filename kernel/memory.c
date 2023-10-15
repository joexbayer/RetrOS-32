/**
 * @file memory.c
 * @author Joe Bayer (joexbayer)
 * @brief A primitiv memory allocation program and virtual memory functions.
 * @version 0.3
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

/* prototypes */
void init_memory();
void* kalloc(int size);
void kfree(void* ptr);
/* ... */

/**
 * @brief Free function for the heap allocation
 * 
 * @warning is thread safe
 * @param ptr Pointer to the memory to free
 * @return void
 */
void free(void* ptr)
{
	dbgprintf("Freeing %x\n", ptr);
	if(ptr == NULL)
		return;
		
	/* lock on free as multiple threads can free at the same time */
	spin_lock(&current_running->allocations->spinlock);
	
	vmem_stack_free(current_running, ptr);	

	spin_unlock(&current_running->allocations->spinlock);
}

void* malloc(unsigned int size)
{	
	if (size <= 0){
		return NULL;
	}
	
	/* lock on malloc as multiple threads can malloc at the same time */
	spin_lock(&current_running->allocations->spinlock);

	void* ptr = vmem_stack_alloc(current_running, size);
	if(ptr == NULL){
		spin_unlock(&current_running->allocations->spinlock);
		return NULL;
	}

	vmem_dump_heap(current_running->allocations->head);

	spin_unlock(&current_running->allocations->spinlock);
	return ptr;
}

void* calloc(int size, int val)
{
	void* m = malloc(size);
	if(m == NULL)	
		return NULL;

	memset(m, val, size);
	return m;
}

error_t get_mem_info(struct mem_info* info)
{
	struct mem_info inf = {
		.kernel.total = kmemory_total(),
		.kernel.used = kmemory_used(),
		.permanent.total = PMEM_END_ADDRESS-PERMANENT_KERNEL_MEMORY_START,
		.permanent.used = pmemory_used()
	};

	*info = inf;

	return 0;
}

/**
 * @brief Initializes all memory chunks and sets them to be free.
 * 
 * @return void
 */
void init_memory()
{
	kmem_init();
	dbgprintf("Kernel memory initiated\n");
	vmem_init();
	dbgprintf("Virtual memory initiated\n");
	vmem_init_kernel();
	dbgprintf("Virtual Kernel memory initiated\n");
}