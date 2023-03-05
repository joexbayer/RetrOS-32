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

static int memory_process_used = 0;
void free(void* ptr)
{
	if(ptr == current_running->allocations->address){
		struct allocation* old = current_running->allocations;
		current_running->allocations = current_running->allocations->next;
		current_running->used_memory -= old->size;
		memory_process_used -= old->size;
		
		vmem_free_allocation(old);
		return;
	}

	struct allocation* iter = current_running->allocations;
	while(iter->next != NULL){
		if(iter->next->address == ptr){
			
			struct allocation* save = iter->next;
			iter->next = iter->next->next;
			current_running->used_memory -= save->size;
			memory_process_used -= save->size;

			vmem_free_allocation(save);
			return;
		}
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
		
		allocation->address = (uint32_t*) VMEM_HEAP;

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
/**
 * @brief Initializes all memory chunks and sets them to be free.
 * 
 * @return void
 */
void init_memory()
{
	kmem_init();
	vmem_init();
	vmem_init_kernel();
}