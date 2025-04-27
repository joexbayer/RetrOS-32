/**
 * @file memory.c
 * @author Joe Bayer (joexbayer)
 * @brief A primitive memory allocation program and virtual memory functions.
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
#include <kutils.h>

#define MB(mb) (mb*1024*1024)
#define KB(kb) (kb*1024)

static struct memory_map kernel_memory_map = {0};
static int memory_test(){
	for (int i = 0; i < (15 * 1024*1024)+(1*1024*1024); i++){
		volatile char value = *(volatile char *)i;
		*(volatile char *)i = value;

		if (i % (1024*1024) == 0){
			dbgprintf("[KERNEL] 0x%x MB tested\n", i);
		}
	}
	return 0;
}

error_t get_mem_info(struct mem_info* info)
{
	*info = (struct mem_info) {
		.kernel.total = memory_map_get()->kernel.total,
		.kernel.used = kmemory_used(),
		.permanent.total = memory_map_get()->permanent.total,
		.permanent.used = pmemory_used(),
		.virtual_memory.total = memory_map_get()->virtual_memory.total,
		.virtual_memory.used = vmem_total_usage(),
	};
	return 0;
}


int memory_map_init(int total_memory, int extended_memory)
{
	/* memory starts at 1MB */ 
	uintptr_t start = MB(1);

	int permanent = ALIGN((int)(total_memory * (float)1/8), 1024*1024);
	int kernel = ALIGN((int)(total_memory * (float)2/8), 1024*1024);

	/* Calculate the remaining memory */
	int remaining_memory = total_memory - permanent - kernel;
	int virtual = ALIGN_DOWN(remaining_memory, 4096);

	if (permanent + kernel + virtual > total_memory) {
		virtual = total_memory - permanent - kernel;
		virtual = ALIGN_DOWN(virtual, 4096);
	}

	kernel_memory_map = (struct memory_map) {
		.kernel.from 			= start,
		.kernel.to 				= start + kernel,
		.kernel.total 			= kernel,

		.permanent.from 		= start + kernel,
		.permanent.to 			= start + kernel + permanent,
		.permanent.total 		= permanent,

		.virtual_memory.from 	= start + kernel + permanent,
		.virtual_memory.to 		= start + kernel + permanent + virtual,
		.virtual_memory.total 	= virtual,

		.total 					= permanent+kernel+virtual,
		.initialized 			= true
	};
	
	return 0;
}

struct memory_map* memory_map_get()
{
	return &kernel_memory_map;
}

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
	if(ptr == NULL)return;
		
	/* lock on free as multiple threads can free at the same time */
	spin_lock(&($process->current->allocations->spinlock));	
	vmem_stack_free($process->current, ptr);	
	spin_unlock(&($process->current->allocations->spinlock));
}

void* malloc(unsigned int size)
{	
	if (size <= 0){
		return NULL;
	}
	
	size = ALIGN(size, PTR_SIZE);

	/* lock on malloc as multiple threads can malloc at the same time */
	spin_lock(&($process->current->allocations->spinlock));

	/* No need to check if ptr is null as we simply return it */
	void* ptr = vmem_stack_alloc($process->current, size);

	spin_unlock(&($process->current->allocations->spinlock));
	return ptr;
}

void* calloc(int size, int val)
{
	void* m = malloc(size);
	if(m == NULL) return NULL;

	memset(m, val, size);
	return m;
}

void* krealloc(void* ptr, int new_size)
{
    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return kalloc(new_size);
    }

    void *new_ptr = kalloc(new_size);
    if (new_ptr == NULL) {
        return NULL;
    }

    memcpy(new_ptr, ptr, new_size);

    kfree(ptr);
    return new_ptr;
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
	memory_test();
}