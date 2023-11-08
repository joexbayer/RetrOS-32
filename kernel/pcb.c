/**
 * @file pcb.c
 * @author Joe Bayer (joexbayer)
 * @brief Manages PCBs creation, destruction and their status.
 * @version 0.1
 * @date 2022-06-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <arch/gdt.h>
#include <pcb.h>
#include <serial.h>
#include <memory.h>
#include <scheduler.h>
#include <fs/ext.h>
#include <assert.h>
#include <kthreads.h>
#include <kutils.h>
#include <util.h>
#include <errors.h>

#include <fs/fs.h>

static struct pcb pcb_table[MAX_NUM_OF_PCBS];
/**
 * Current running PCB, used for context aware
 * functions such as windows drawing to the screen.
 */
struct pcb* current_running = &pcb_table[0];

//#include <gfx/gfxlib.h>

const char* pcb_status[] = {"stopped ", "running ", "new     ", "blocked ", "sleeping", "zombie"};
static int pcb_count = 0;

static void __pcb_free(struct pcb* pcb)
{
	memset(pcb, 0, sizeof(struct pcb));
	pcb->state = STOPPED;
	pcb->pid = -1;
	pcb->next = NULL;
	pcb->prev = NULL;
}

static int __pcb_load_from_disk(const char* file, void* buf, int size)
{
	inode_t inode = fs_open(file, FS_FILE_FLAG_READ);
	if(inode < 0){
		dbgprintf("Error opening %s\n", file);
		return -ERROR_FILE_NOT_FOUND;
	}

	dbgprintf("Reading %s from disk\n", file);
	int read = fs_read(inode, buf, size);
	if(read < 0){
		fs_close(inode);
		return -ERROR_FILE_NOT_FOUND;
	}

	fs_close(inode);
	return read;
}

static int __pcb_init_kernel_stack(struct pcb* pcb)
{
	int32_t stack = (uint32_t) kalloc(PCB_STACK_SIZE);
	if((void*)stack == NULL){
		return -ERROR_ALLOC;
	}
	memset((void*)stack, 0, PCB_STACK_SIZE);

	/* Stack grows down so we want the upper part of allocated memory.*/ 
	pcb->ctx.ebp = stack+PCB_STACK_SIZE-1;
	pcb->ctx.esp = stack+PCB_STACK_SIZE-1;
	pcb->kesp = pcb->ctx.esp;
	pcb->kebp = pcb->kesp;
	pcb->stackptr = stack;

	return ERROR_OK;
}

static struct pcb* __pcb_get_free()
{
	ASSERT_CRITICAL();
	int i; /* Find a pcb is that is "free" */
	for(i = 0; i < MAX_NUM_OF_PCBS; i++)
		if(pcb_table[i].state == STOPPED){
			struct pcb* pcb = &pcb_table[i];

			pcb->pid = i;
			pcb->state = PCB_NEW;
			pcb->used_memory = 0;
			pcb->kallocs = 0;
			pcb->preempts = 0;
			pcb->is_process = 0;
			pcb->args = 0;
			pcb->argv = NULL;
			pcb->current_directory = 0;
			pcb->yields = 0;

			return pcb;
		}

	return NULL;
}

static struct pcb* __pcb_init_process(byte_t flags, uint32_t entry_point)
{
    ENTER_CRITICAL();

    struct pcb* pcb = __pcb_get_free();
    if (pcb == NULL) {
        LEAVE_CRITICAL();
        return NULL;
    }

    if (__pcb_init_kernel_stack(pcb) < 0) {
        __pcb_free(pcb);
        LEAVE_CRITICAL();
        return NULL;
    }

	pcb->ctx.esp = 0xEFFFFFF0;
    pcb->ctx.ebp = pcb->ctx.esp;
    pcb->ctx.eip = entry_point;
    pcb->is_process = 1;
    pcb->cs = GDT_PROCESS_CS | PROCESSS_PRIVILEGE;
    pcb->ds = GDT_PROCESS_DS | PROCESSS_PRIVILEGE;

    /* Set kernel privileges if flag is set */
    if (flags & PCB_FLAG_KERNEL) {
        pcb->cs = GDT_KERNEL_CS;
        pcb->ds = GDT_KERNEL_DS;
    }

    LEAVE_CRITICAL();
    return pcb;
}

static int __pcb_init_virt_args(struct pcb* pcb, int argc, char** argv)
{
	if(argc == 0) return ERROR_OK;

	struct args* virtual_args = vmem_stack_alloc(pcb, sizeof(struct args));
	if(virtual_args == NULL){
		dbgprintf("[PCB] Failed to allocate memory for virtual args\n");
		return -ERROR_ALLOC;
	}

	/* get physical address */
	uint32_t* heap_table = (uint32_t*)(pcb->page_dir[DIRECTORY_INDEX(VMEM_HEAP)] & ~PAGE_MASK);
	uint32_t heap_page = (uint32_t)((uint32_t*)heap_table)[TABLE_INDEX((uint32_t)virtual_args)]& ~PAGE_MASK;

	struct args* _args = (struct args*)(heap_page);
	/* copy over args */
	_args->argc = argc;
	for (int i = 0; i < argc; i++){
		memcpy(_args->data[i], argv[i], strlen(argv[i])+1);
		_args->argv[i] = virtual_args->data[i];
		dbgprintf("Arg %d: %s (0x%x)\n", i, _args->data[i], _args->argv[i]);
	}
	pcb->args = _args->argc;
	pcb->argv = virtual_args->argv;

	return ERROR_OK;
}

/**
 * @brief Sets all PCBs state to stopped. Meaning they can be started.
 * Also starts the PCB background process.
 */
void init_pcbs()
{   

	/* Stopped processes are eligible to be "replaced." */
	for (int i = 0; i < MAX_NUM_OF_PCBS; i++){
		pcb_table[i].state = STOPPED;
		pcb_table[i].pid = -1;
		pcb_table[i].next = NULL;
	}

	current_running = &pcb_table[0];

	dbgprintf("[PCB] All process control blocks are ready.\n");
}

int pcb_total_usage()
{
	int total = 0;
	/* Do not include idle task at pid 0 */
	for (int i = 1; i < MAX_NUM_OF_PCBS; i++){
		total += pcb_table[i].preempts;
	}
	return total;
}

error_t pcb_get_info(int pid, struct pcb_info* info)
{
	if(pid < 0 || pid > MAX_NUM_OF_PCBS || pcb_table[pid].state == STOPPED)
		return -ERROR_INDEX;

	struct pcb_info _info = {
		.pid = pid,
		.stack = pcb_table[pid].ctx.esp,
		.state = pcb_table[pid].state,
		.used_memory = pcb_table[pid].used_memory,
		.is_process = pcb_table[pid].is_process,
		.usage = (float)pcb_table[pid].preempts / (float)pcb_total_usage(),
		.name = {0}
	};
	memcpy(_info.name, pcb_table[pid].name, PCB_MAX_NAME_LENGTH);

	*info = _info;

	return ERROR_OK;
}

void pcb_kill(int pid)
{
	if(pid < 0 || pid > MAX_NUM_OF_PCBS) return;
	pcb_table[pid].state = ZOMBIE;
}

void Genesis()
{
	while (1);
}

void idletask(){
	dbgprintf("Hello world!\n");
	while(1){
		HLT();
	};
}

struct pcb* pcb_get_by_pid(int pid)
{
	return &pcb_table[pid];
}


void pcb_dbg_print(struct pcb* pcb)
{
	dbgprintf("\n###### PCB ######\npid: %d\nname: %s\nesp: 0x%x\nebp: 0x%x\nkesp: 0x%x\nkebp: 0x%x\neip: 0x%x\nstate: %s\nstack limit: 0x%x\nstack size: 0x%x (0x%x - 0x%x)\nPage Directory: 0x%x\nCS: %d\nDS:%d\n",
		pcb->pid, pcb->name, pcb->ctx.esp, pcb->ctx.ebp, pcb->kesp, pcb->kebp, pcb->ctx.eip, pcb_status[pcb->state], pcb->stackptr, (int)((pcb->stackptr+PCB_STACK_SIZE-1) - pcb->ctx.esp), (pcb->stackptr+PCB_STACK_SIZE-1), pcb->ctx.esp,  pcb->page_dir, pcb->cs, pcb->ds
	);
}

/**
 * @brief Sets the process with given pid to stopped. Also frees the process's stack.
 * 
 * @param pid id of the process.
 * @return int index of pcb, -1 on error.
 */
int pcb_cleanup_routine(int pid)
{
	assert(pid != current_running->pid && !(pid < 0 || pid > MAX_NUM_OF_PCBS));

	dbgprintf("[PCB] Cleanup on PID %d stack: 0x%x (original: 0x%x)\n", pid, pcb_table[pid].ctx.esp, pcb_table[pid].stackptr+PCB_STACK_SIZE-1);

	gfx_destory_window(pcb_table[pid].gfx_window);
	
	/* kthreads only free their virtual allocations */
	if(pcb_table[pid].is_process){
		vmem_cleanup_process(&pcb_table[pid]);
	} else {
		vmem_free_allocations(&pcb_table[pid]);
	}
	kfree((void*)pcb_table[pid].stackptr);

	pcb_count--;

	CRITICAL_SECTION({
		memset(&pcb_table[pid], 0, sizeof(struct pcb));
		pcb_table[pid].state = STOPPED;
		pcb_table[pid].pid = -1;
	});

	dbgprintf("[PCB] Cleanup on PID %d [DONE]\n", pid);

	return pid;
}

/**
 * @brief Creates a new kernel thread.
 * Creates a process thread which inherets the parents virtual memory.
 * @param entry entry function
 * @param name name of thread
 * @param flags flags for thread
 * @return int pid of thread, -1 on error.
 */
error_t pcb_create_thread(struct pcb* parent, void (*entry)(), char* name, byte_t flags)
{

	/* Initialize PCB and set privileges */
    struct pcb* pcb = __pcb_init_process(flags, (uint32_t)entry);
	if(pcb == NULL){
        return -ERROR_NULL_POINTER;
    }
    
    /* Inherit parent's attributes */
    pcb->term = parent->term;
    pcb->parent = current_running;

	/* inheret parents virtual memory */
	vmem_init_process_thread(parent, pcb);

	get_scheduler()->ops->add(get_scheduler(), pcb);

	pcb_count++;
	return pcb->pid;
}

error_t pcb_create_process(char* program, int argc, char** argv, pcb_flag_t flags)
{
	char* buf;
	int ret, size;
	struct pcb* pcb;

	/* Load program into memory */
	buf = kalloc(MAX_FILE_SIZE);
	if(buf == NULL){return -ERROR_ALLOC;}
	
	ret = __pcb_load_from_disk(program, buf, MAX_FILE_SIZE);
	if(ret < 0){
		dbgprintf("Error loading %s\n", program);
		
		kfree(buf);
		return -ERROR_FILE_NOT_FOUND;
	}
	size = ret;

	pcb = __pcb_init_process(flags, VMEM_DATA);
	if(pcb == NULL){
		kfree(buf);
        return -ERROR_NULL_POINTER;
    }

	pcb->data_size = size;
	memcpy(pcb->name, program, strlen(program)+1);

	pcb->term = current_running->term;
	pcb->parent = current_running;

	/* Memory map data */
	vmem_init_process(pcb, buf, size);

	ret = __pcb_init_virt_args(pcb, argc, argv);
	if(ret < 0){
		kfree(buf);
		__pcb_free(pcb);

		return -ERROR_ALLOC;
	}

	get_scheduler()->ops->add(get_scheduler(), pcb);
	// TODO: Check for errors

	pcb_count++;
	kfree(buf);

	dbgprintf("Created new process!\n");
	/* Run */
	return pcb->pid;
}

/**
 * @brief Add a pcb to the list of running proceses.
 * Also instantiates the PCB itself.
 * 
 * @param entry pointer to entry function.
 * @param name name of process
 * @return int amount of running processes, -1 on error.
 */
error_t pcb_create_kthread(void (*entry)(), char* name)
{   
	ENTER_CRITICAL();
	
	struct pcb* pcb = __pcb_get_free();
	if(pcb == NULL){
		dbgprintf("No free PCBs!\n");

		LEAVE_CRITICAL();
		return -ERROR_PCB_FULL;
	}
	
	/* Set up stack */
	int ret = __pcb_init_kernel_stack(pcb);
	if(ret < 0){
		dbgprintf("[PCB] Failed to allocate stack\n");
		return -ERROR_ALLOC;
	}

	pcb->parent = current_running;
	pcb->term = current_running->term;

	pcb->thread_eip = (uintptr_t) entry;
	pcb->page_dir = kernel_page_dir;
	pcb->ctx.eip = (uint32_t)&kthread_entry;

	pcb->cs = GDT_KERNEL_CS;
	pcb->ds = GDT_KERNEL_DS;

	memcpy(pcb->name, name, strlen(name)+1);
	
	pcb->page_dir = kernel_page_dir;
	/* this is done for processes in vmem.c, should probably be moved there? */
	pcb->allocations = kalloc(sizeof(struct virtual_allocations));
	if(pcb->allocations == NULL){
		__pcb_free(pcb);
		dbgprintf("[PCB] Failed to allocate memory for virtual allocations\n");
		LEAVE_CRITICAL();
		return -ERROR_ALLOC;
	}

	pcb->allocations->head = NULL;
	pcb->allocations->spinlock = 0;

	get_scheduler()->ops->add(get_scheduler(), pcb);

	pcb_count++;
	dbgprintf("Added %s, PID: %d, Stack: 0x%x\n", name, pcb->pid, pcb->kesp);
	LEAVE_CRITICAL();
	return pcb->pid;
}

void __noreturn start_pcb(struct pcb* pcb)
{   
	pcb->state = RUNNING;
	dbgprintf("[START PCB] Starting pcb!\n");
	_start_pcb(pcb); /* asm function */
	
	UNREACHABLE();
}
