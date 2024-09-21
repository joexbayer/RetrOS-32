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
#include <libc.h>
#include <errors.h>

#include <syscalls.h>
#include <syscall_helper.h>

#include <fs/fs.h>

#include <user.h>
#include <admin.h>
#include <usermanager.h>

static struct pcb pcb_table[MAX_NUM_OF_PCBS];
static struct process __process = {
	.current = &(struct pcb){
		.name = "kernel",
		.pid = 0,
		.user = &(struct user){
			.name = "system",
			.permissions = SYSTEM_FULL_ACCESS,
		}
	},
};
struct process* $process = &__process;

/**
 * Current running PCB, used for context aware
 * functions such as windows drawing to the screen.
 */
const char* pcb_status[] = {"stopped ", "running ", "new     ", "blocked ", "sleeping", "zombie"};
static int pcb_count = 0;

static void __pcb_free(struct pcb* pcb)
{
	memset(pcb, 0, sizeof(struct pcb));
	pcb->state = STOPPED;
	pcb->parent = NULL;
	pcb->pid = -1;
	pcb->next = NULL;
	pcb->prev = NULL;
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

static int pcb_kthread_free_args(struct pcb* pcb)
{
	ERR_ON_NULL(pcb);

	if(pcb->args == 0) return ERROR_OK;

	for (int i = 0; i < pcb->args; i++){
		kfree(pcb->argv[i]);
	}
	kfree(pcb->argv);

	return ERROR_OK;
}

static error_t pcb_kthread_init_args(struct pcb* pcb, int argc, char** argv)
{	
	ERR_ON_NULL(pcb);
	if(argc == 0) return ERROR_OK;

	dbgprintf("[PCB] Allocating %d args\n", argc);

	/* copy and allocate space for args */
	pcb->argv = kalloc(sizeof(char*)*argc);
	if(pcb->argv == NULL){
		return -ERROR_ALLOC;
	}

	for (int i = 0; i < argc; i++){
		pcb->argv[i] = kalloc(100);
		if(pcb->argv[i] == NULL){
			return -ERROR_ALLOC;
		}
		memcpy(pcb->argv[i], argv[i], strlen(argv[i])+1);	
	}

	pcb->args = argc;

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
			pcb->in_kernel = false;

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
	pcb->thread_eip = 0;

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
	memcpy(_info.user, pcb_table[pid].user->name, USER_MAX_NAME_LENGTH);

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

struct pcb* pcb_get_by_name(char* name)
{
	for (int i = 0; i < MAX_NUM_OF_PCBS; i++){
		if(strncmp(pcb_table[i].name, name, strlen(name)) == 0){
			return &pcb_table[i];
		}
	}
	return NULL;
}

int pcb_await(int pid)
{
	if(pid < 0 || pid > MAX_NUM_OF_PCBS) return -1;
	while(pcb_table[pid].state != STOPPED){
		kernel_yield();
	}
	return 0;
}
EXPORT_SYSCALL(SYSCALL_AWAIT_PROCESS, pcb_await);


void pcb_dbg_print(struct pcb* pcb)
{
	dbgprintf("\n###### PCB ######\npid: %d\nname: %s\nesp: 0x%x\nebp: 0x%x\nkesp: 0x%x\nkebp: 0x%x\neip: 0x%x\nstate: %s\nstack limit: 0x%x\nstack size: 0x%x (0x%x - 0x%x)\nPage Directory: 0x%x\nCS: %d\nDS:%d\n",
		pcb->pid, pcb->name, pcb->ctx.esp, pcb->ctx.ebp, pcb->kesp, pcb->kebp, pcb->ctx.eip, pcb_status[pcb->state], pcb->stackptr, (int)((pcb->stackptr+PCB_STACK_SIZE-1) - pcb->ctx.esp), (pcb->stackptr+PCB_STACK_SIZE-1), pcb->ctx.esp,  pcb->page_dir, pcb->cs, pcb->ds
	);
}

/**
 * @brief Sets the process with given pid to stopped. Also frees the process's stack.
 * 
 * @param void* arg pid of process to stop.
 * @return int index of pcb, -1 on error.
 */
int pcb_cleanup_routine(void* arg)
{
	AUTHORIZED_GUARD(SYSTEM_FULL_ACCESS);

	int pid = (int)arg;
	assert(pid != $process->current->pid && !(pid < 0 || pid > MAX_NUM_OF_PCBS));

	dbgprintf("%d\n", __cli_cnt);
	struct pcb* pcb = &pcb_table[pid];

	if(pcb->gfx_window != NULL){
		gfx_destory_window(pcb_table[pid].gfx_window);
	}

	/**
	 * @brief A process cannot exit before all its children have exited.
	 * Therefor loop over all pcbs and kill them if current is their parent.
	 */
	for (int i = 0; i < MAX_NUM_OF_PCBS; i++){
		if(pcb_table[i].parent == pcb && pcb_table[i].is_process == PCB_THREAD){
			pcb_table[i].state = ZOMBIE;
		}
	}
	
	switch (pcb->is_process){
	case PCB_PROCESS:
		vmem_cleanup_process(pcb);
		break;
	case PCB_THREAD:
		vmem_cleanup_process_thead(pcb);
		break;
	default:{
			pcb_kthread_free_args(pcb);
			vmem_free_allocations(pcb);
		}
		break;
	}
	kfree((void*)pcb->stackptr);

	pcb_count--;

	CRITICAL_SECTION({
		__pcb_free(pcb);
	});

	dbgprintf("[PCB] Cleanup on PID %d [DONE]\n", pid);

	dbgprintf("%d\n", __cli_cnt);

	return pid;
}

/**
 * @brief Creates a new kernel thread.
 * Creates a process thread which inherets the parents virtual memory.
 * In the context of a kernel thread the thread_eip is set to the entry function.
 * @param entry entry function
 * @param name name of thread
 * @param flags flags for thread
 * @return int pid of thread, -1 on error.
 */
error_t pcb_create_thread(struct pcb* parent, void (*entry)(), void* arg, byte_t flags)
{
	AUTHORIZED_GUARD(CTRL_PROC_CREATE | SYSTEM_FULL_ACCESS | ADMIN_FULL_ACCESS);

	/* Initialize PCB and set privileges */
    struct pcb* pcb = __pcb_init_process(flags, VMEM_DATA);
	if(pcb == NULL){
        return -ERROR_NULL_POINTER;
    }
    
    /* Inherit parent's attributes */
    pcb->term = parent->term;
	pcb->user = parent->user;
    pcb->parent = parent;
	pcb->thread_eip = (uintptr_t) entry;
	pcb->is_process = PCB_THREAD;

	/**
	 * @brief This is kinda a hack
	 * To pass the thread argument we reuse the args field.
	 * As its the first argument of the thread function.
	 */
	pcb->args = (uint32_t)arg;

	/* name */
	memcpy(pcb->name, parent->name, strlen(parent->name)+1);

	/* inheret parents virtual memory */
	vmem_init_process_thread(parent, pcb);

	get_scheduler()->ops->add(get_scheduler(), pcb);

	pcb_count++;
	return pcb->pid;
}

error_t pcb_create_process(char* program, int argc, char** argv, pcb_flag_t flags)
{
	AUTHORIZED_GUARD(CTRL_PROC_CREATE | SYSTEM_FULL_ACCESS | ADMIN_FULL_ACCESS);

	char* buf;
	int ret, size;
	struct pcb* pcb;

	/* Load program into memory */
	buf = kalloc(64*1024);
	if(buf == NULL){return -ERROR_ALLOC;}
	
	ret = fs_load_from_file(program, buf, 64*1024);
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

	pcb->term = $process->current->term;
	pcb->user = $process->current->user;
	pcb->parent = $process->current;

	pcb->thread_eip = 0;

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
error_t pcb_create_kthread(void (*entry)(), char* name, int argc, char** argv)
{   

	AUTHORIZED_GUARD(CTRL_PROC_CREATE | SYSTEM_FULL_ACCESS | ADMIN_FULL_ACCESS);

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

	pcb->parent = NULL;
	pcb->term = $process->current->term;
	pcb->user = $process->current->user;

	pcb->thread_eip = (uintptr_t) entry;
	pcb->page_dir = kernel_page_dir; /* This should probably be in some global $ config */
	pcb->ctx.eip = (uint32_t)&kthread_entry;
	
	/* kthreads always run in kernel */
	pcb->in_kernel = true;

	pcb->cs = GDT_KERNEL_CS;
	pcb->ds = GDT_KERNEL_DS;

	memcpy(pcb->name, name, strlen(name)+1);
	
	/* this is done for processes in vmem.c, should probably be moved there? */
	pcb->allocations = create(struct virtual_allocations);
	if(pcb->allocations == NULL){
		__pcb_free(pcb);
		dbgprintf("[PCB] Failed to allocate memory for virtual allocations\n");
		LEAVE_CRITICAL();
		return -ERROR_ALLOC;
	}

	ret = pcb_kthread_init_args(pcb, argc, argv);
	if(ret < 0){
		__pcb_free(pcb);
		dbgprintf("[PCB] Failed to allocate memory for args\n");
		LEAVE_CRITICAL();
		return -ERROR_ALLOC;
	}

	get_scheduler()->ops->add(get_scheduler(), pcb);

	pcb_count++;
	
	dbgprintf("Added %s, PID: %d, Stack: 0x%x\n", name, pcb->pid, pcb->kesp);
	LEAVE_CRITICAL();
	return pcb->pid;
}

void __noreturn start_pcb(struct pcb* pcb)
{   
	pcb->state = RUNNING;
	_start_pcb(pcb); /* asm function */
	
	UNREACHABLE();
}
