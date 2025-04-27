#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <pcb.h>
#include <errors.h>
#include <kutils.h>
#include <sync.h>

#define create(type) ((type *)kcalloc(sizeof(type)))

extern byte_t _code[], _end[], _code_end[], _ro_s[], _ro_e[], _data_s[], _data_e[], _bss_s[], _bss_e[], _bss_size[];
extern int kernel_size;

#define MB(mb) (mb*1024*1024)
#define KB(kb) (kb*1024)


#define PERMANENT_KERNEL_MEMORY_START 0x100000
#define PMEM_END_ADDRESS 	 0x200000


#define VMEM_STACK          0xEFFFFFF0
#define VMEM_HEAP           0xE0000000
#define VMEM_DATA           0x1000000

#define SUPERVISOR          0
#define PRESENT             1
#define READ_WRITE          2
#define USER                4

#define WRITE_THROUGH       8
#define ACCESSED            32

#define PAGE_DIRECTORY_BITS 22
#define PAGE_TABLE_BITS     12
#define PAGE_TABLE_MASK     0x000003ff
#define PAGE_SIZE           0x1000
#define PAGE_MASK           0xfff

extern uint32_t* kernel_page_dir;

struct mem_info {
	struct kernel {
		int used;
		int total;
	} kernel;
	struct _virtual {
		int used;
		int total;
	}virtual_memory;
	struct permanent {
		int used;
		int total;
	}permanent;
};

struct memory_map {
	struct kernel_memory {
		uintptr_t from;
		uintptr_t to;
		int total;
	} kernel;
	struct permanent_memory {
		uintptr_t from;
		uintptr_t to;
		int total;
	} permanent;
	struct virtual_memory {
		uintptr_t from;
		uintptr_t to;
		int total;
	} virtual_memory;
	int total;
	bool_t initialized;
};
struct memory_map* memory_map_get();


struct vmem_page_region {
	int* bits;
	uint32_t* basevaddr;
	int refs;

	int size;
	int used;
};

struct virtual_allocations {
	struct allocation* head;
	struct allocation* tail;

	spinlock_t spinlock;
};

struct allocation {
	int* bits;
	uint32_t* address;
	int size;
	int used;
	struct vmem_page_region* region;
	struct allocation* next;
};

#define TABLE_INDEX(vaddr) ((vaddr >> PAGE_TABLE_BITS) & PAGE_TABLE_MASK)
#define DIRECTORY_INDEX(vaddr) ((vaddr >> PAGE_DIRECTORY_BITS) & PAGE_TABLE_MASK)

int memory_map_init(int total_memory, int extended_memory);
void init_memory();
error_t get_mem_info(struct mem_info* info);
void kmem_init();
void vmem_init();

/* Kernel memory*/
void* kalloc(int size);
void* kcalloc(int size);
void* krealloc(void* ptr, int new_size);
void kfree(void* ptr);
int kmemory_used();
int kmemory_total();

/* Permanent memory */
void* palloc(int size);
int pmemory_used();

/* Userspace memory */
void* malloc(unsigned int size);
void free(void* ptr);

/* Assembly helper functions */
void load_page_directory();
void enable_paging();

/* Virtual memory API */
void vmem_map_driver_region(uint32_t addr, int size);
void vmem_init_kernel();

void vmem_cleanup_process(struct pcb* pcb);
void vmem_cleanup_process_thead(struct pcb* thread);

void vmem_init_process_thread(struct pcb* parent, struct pcb* thread);
void vmem_init_process(struct pcb* pcb, byte_t* data, int size);
void vmem_stack_free(struct pcb* pcb, void* ptr);
void* vmem_stack_alloc(struct pcb* pcb, int size);
void vmem_dump_heap(struct allocation* allocation);

int vmem_total_usage();

int vmem_free_allocations(struct pcb* pcb);

void vmem_free_allocation(struct allocation* allocation);
int vmem_continuous_allocation_map(struct pcb* pcb, struct allocation* allocation, uint32_t* address, int num, int access);
#endif