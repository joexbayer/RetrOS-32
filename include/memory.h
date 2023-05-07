#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <pcb.h>
#include <errors.h>

extern char _code[], _end[], _code_end[], _ro_s[], _ro_e[], _data_s[], _data_e[], _bss_s[], _bss_e[];
extern int kernel_size;

#define PMEM_END_ADDRESS 	 0x2000000

#define VMEM_MAX_ADDRESS    0x1600000
#define VMEM_START_ADDRESS  0x400000
#define VMEM_TOTAL_PAGES ((VMEM_MAX_ADDRESS-VMEM_START_ADDRESS) / PAGE_SIZE)

#define VMEM_MANAGER_START  0x200000
#define VMEM_MANAGER_END    0x300000
#define VMEM_MANAGER_PAGES ((VMEM_MANAGER_END-VMEM_MANAGER_START) / PAGE_SIZE)

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
	struct virtual {
		int used;
		int total;
	}virtual;
	struct permanent {
		int used;
		int total;
	}permanent;
};

struct allocation {
	int* bits;
	uint32_t* address;
	int size;
	struct allocation* next;
};

#define TABLE_INDEX(vaddr) ((vaddr >> PAGE_TABLE_BITS) & PAGE_TABLE_MASK)
#define DIRECTORY_INDEX(vaddr) ((vaddr >> PAGE_DIRECTORY_BITS) & PAGE_TABLE_MASK)

void init_memory();
error_t get_mem_info(struct mem_info* info);
void kmem_init();
void vmem_init();

/* Kernel memory*/
void* kalloc(int size);
void kfree(void* ptr);
int kmemory_used();
int kmemory_total();

/* Permanent memory */
void* palloc(int size);

/* Userspace memory */
void* malloc(unsigned int size);
void free(void* ptr);

/* Assembly helper functions */
void flush_tlb_entry(uint32_t vaddr);
void load_page_directory();
void enable_paging();

/* Virtual memory API */
void vmem_map_driver_region(uint32_t addr, int size);
void vmem_init_kernel();
void vmem_cleanup_process(struct pcb* pcb);
void vmem_init_process(struct pcb* pcb, char* data, int size);

void vmem_free_allocation(struct allocation* allocation);
int vmem_continious_allocation_map(struct allocation* allocation, uint32_t* address, int num, int access);

#endif