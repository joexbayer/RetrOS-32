/**
 * @file kmem.c
 * @author Joe Bayer (joexbayer)
 * @brief Kernel memory allocator
 * @version 0.1
 * @date 2023-03-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <kconfig.h>
#include <memory.h>
#include <serial.h>
#include <sync.h>
#include <bitmap.h>
#include <assert.h>
#include <ksyms.h>


#ifndef KDEBUG_MEMORY
#undef dbgprintf
#define dbgprintf(...)
#endif

/* Dynamic kernel memory */

#define KMEM_BLOCK_SIZE 		256
#define KMEM_BLOCKS_PER_BYTE 	8
#define KMEM_BITMAP_INDEX(addr) ((addr - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE / KMEM_BLOCKS_PER_BYTE)
#define KMEM_BITMAP_OFFSET(addr) ((addr - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE % KMEM_BLOCKS_PER_BYTE)

/* values determined by memory map, set at runtime */
static uint32_t KERNEL_MEMORY_START = 0;
static uint32_t KERNEL_MEMORY_END = 0;

static uint8_t*   __kmemory_bitmap;
static spinlock_t __kmemory_lock = 0;
static uint32_t   __kmemory_used = 0;

static inline int __kmemory_find_blocks(int num_blocks, int total_blocks)
{
    int free_blocks = 0;
    for (int i = 0; i < total_blocks; i++) {
        uint32_t index = KMEM_BITMAP_INDEX(KERNEL_MEMORY_START + i * KMEM_BLOCK_SIZE);
        uint32_t offset = KMEM_BITMAP_OFFSET(KERNEL_MEMORY_START + i * KMEM_BLOCK_SIZE);
        if (!(__kmemory_bitmap[index] & (1 << offset))) {
            free_blocks++;
            if (free_blocks == num_blocks) {
                return i - num_blocks + 1;
            }
        } else {
            free_blocks = 0;
        }
    }
    return -1;  /* No free block found */
}

static inline void __kmemory_mark_blocks(int start_block, int num_blocks)
{
    for (int i = start_block; i < start_block + num_blocks; i++) {
        uint32_t index = KMEM_BITMAP_INDEX(KERNEL_MEMORY_START + i * KMEM_BLOCK_SIZE);
        uint32_t offset = KMEM_BITMAP_OFFSET(KERNEL_MEMORY_START + i * KMEM_BLOCK_SIZE);
        __kmemory_bitmap[index] |= (1 << offset);
    }
}

static inline void __kmemory_write_metadata(int start_block, int num_blocks)
{
    int* metadata = (int*) (KERNEL_MEMORY_START + start_block * KMEM_BLOCK_SIZE);
    *metadata = num_blocks;
}


/**
 * @brief Allocates sequential chunks of fixed size (256 bytes each) from a region of kernel memory.
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
    if (size <= 0) return NULL;

    size = ALIGN(size, PTR_SIZE);
    int num_blocks = (size + sizeof(int) + KMEM_BLOCK_SIZE - 1) / KMEM_BLOCK_SIZE;
    int total_blocks = (KERNEL_MEMORY_END - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE;

	spin_lock(&__kmemory_lock);

    int start_block = __kmemory_find_blocks(num_blocks, total_blocks);
    if (start_block == -1) {
        /* No contiguous free region of memory was found */
        uint32_t *ebp = (uint32_t*) __builtin_frame_address(0);
   	    __backtrace_from((uintptr_t*)ebp);

        warningf("Out of memory: %d\n", __kmemory_used);
        kernel_panic("Out of memory!");
    }

    __kmemory_mark_blocks(start_block, num_blocks);
    __kmemory_write_metadata(start_block, num_blocks);

    void* ptr = (void*)(KERNEL_MEMORY_START + start_block * KMEM_BLOCK_SIZE + sizeof(int));

    __kmemory_used += num_blocks * KMEM_BLOCK_SIZE;
    
    if($process != NULL && $process->current != NULL){
        $process->current->kallocs++;
    }

    /* sanity check */
    if(__kmemory_used > KERNEL_MEMORY_END-KERNEL_MEMORY_START){
        warningf("Out of memory: %d\n", __kmemory_used);
        kernel_panic("Out of memory!");
    }

    spin_unlock(&__kmemory_lock);

    return ptr;
}

void* kcalloc(int size)
{
    void* ptr = kalloc(size);
    if(ptr == NULL) return NULL;

    memset(ptr, 0, size);
    return ptr;
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
void kfree(void* ptr)
{
	if (!ptr) return;
	
	spin_lock(&__kmemory_lock);

	uint32_t ptr_addr = (uint32_t)ptr;
	if(ptr_addr < (KERNEL_MEMORY_START + sizeof(int)) || ptr_addr >= KERNEL_MEMORY_END){
		warningf("[MEMORY] Ignoring out-of-range free at 0x%x\n", ptr_addr);
		spin_unlock(&__kmemory_lock);
		return;
	}

	uint32_t meta_addr = ptr_addr - sizeof(int);
	if(((meta_addr - KERNEL_MEMORY_START) % KMEM_BLOCK_SIZE) != 0){
		warningf("[MEMORY] Ignoring unaligned free at 0x%x\n", ptr_addr);
		spin_unlock(&__kmemory_lock);
		return;
	}

	/* Calculate the index of the block in the memory region */
	int block_index = (meta_addr - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE;
	int total_blocks = (KERNEL_MEMORY_END - KERNEL_MEMORY_START) / KMEM_BLOCK_SIZE;

	/* Read the size of the allocated block from the metadata block */
	int* metadata = (int*)meta_addr;
	int num_blocks = *metadata;
	if(num_blocks <= 0 || block_index + num_blocks > total_blocks){
		warningf("[MEMORY] Ignoring corrupted free at 0x%x (blocks=%d)\n", ptr_addr, num_blocks);
		spin_unlock(&__kmemory_lock);
		return;
	}

	if($process != NULL && $process->current != NULL){
		dbgprintf("[MEMORY] %s freeing %d blocks of data\n", $process->current->name, num_blocks);
	}

	for (int i = 0; i < num_blocks; i++) {
		uint32_t index = KMEM_BITMAP_INDEX(KERNEL_MEMORY_START + (block_index + i) * KMEM_BLOCK_SIZE);
		uint32_t offset = KMEM_BITMAP_OFFSET(KERNEL_MEMORY_START + (block_index + i) * KMEM_BLOCK_SIZE);
		if((__kmemory_bitmap[index] & (1 << offset)) == 0){
			warningf("[MEMORY] Ignoring double free at 0x%x\n", ptr_addr);
			spin_unlock(&__kmemory_lock);
			return;
		}
	}

	/* Mark the blocks as free in the bitmap */
	for (int i = 0; i < num_blocks; i++) {
		uint32_t index = KMEM_BITMAP_INDEX(KERNEL_MEMORY_START + (block_index + i) * KMEM_BLOCK_SIZE);
		uint32_t offset = KMEM_BITMAP_OFFSET(KERNEL_MEMORY_START + (block_index + i) * KMEM_BLOCK_SIZE);
		__kmemory_bitmap[index] &= ~(1 << offset);
	}

    uint32_t freed_bytes = num_blocks * KMEM_BLOCK_SIZE;
    if(__kmemory_used >= freed_bytes){
        __kmemory_used -= freed_bytes;
    } else {
        __kmemory_used = 0;
    }
	spin_unlock(&__kmemory_lock);
}

/**
 * @brief Arena style permanent memory allocation scheme for memory that wont be freed.
 * Mainly by the windowservers framebuffer, and E1000's buffers.
 */
static uintptr_t memory_permanent_start = 0;
static uintptr_t memory_permanent_end = 0;
static spinlock_t memory_permanent_lock = 0;
void* palloc(int size)
{
	if(size <= 0) return NULL;
    if(memory_permanent_start == 0 || memory_permanent_end == 0) return NULL;

    size = ALIGN(size, PTR_SIZE);

	spin_lock(&memory_permanent_lock);

	if(memory_permanent_start + size >= memory_permanent_end){
		spin_unlock(&memory_permanent_lock);
		dbgprintf("[WARNING] Not enough permanent memory!\n");
		return NULL;
	}

	uint32_t new = memory_permanent_start + size;
	memory_permanent_start += size;

	spin_unlock(&memory_permanent_lock);

	return (void*) new;
}

int pmemory_used()
{
    return memory_permanent_start-memory_map_get()->permanent.from;
}

int kmemory_used()
{
    return __kmemory_used;
}

int kmemory_total()
{
    return KERNEL_MEMORY_END-KERNEL_MEMORY_START;
}

void kmem_init()
{
    memory_permanent_start = (uint32_t) memory_map_get()->permanent.from;
    memory_permanent_end = (uint32_t) memory_map_get()->permanent.to;

    KERNEL_MEMORY_START = (uint32_t) memory_map_get()->kernel.from;
    KERNEL_MEMORY_END = (uint32_t) memory_map_get()->kernel.to;

    /* Initialize the bitmap */
    __kmemory_bitmap = palloc((memory_map_get()->kernel.total) / KMEM_BLOCK_SIZE / KMEM_BLOCKS_PER_BYTE);
    assert(__kmemory_bitmap != NULL);
    memset(__kmemory_bitmap, 0, (memory_map_get()->kernel.total) / KMEM_BLOCK_SIZE / KMEM_BLOCKS_PER_BYTE);

	__kmemory_lock = 0;
    dbgprintf("Lock 0x%x initiated by %s\n", &__kmemory_lock, $process->current->name);
}
