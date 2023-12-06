#ifndef C75FAF52_8626_4A84_AB39_A02D9C4E44C9
#define C75FAF52_8626_4A84_AB39_A02D9C4E44C9


#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

void init_memory();
void kmem_init();
void vmem_init();

void* kalloc(int size);
void kfree(void* ptr);

typedef unsigned int        uintptr_t;
typedef int                 intptr_t;

int write_block(char* buf, int block);
int read_block(char* buf, int block);

struct pcb {

};

extern struct pcb* current_running;

extern int failed;

void testprintf(int test,  const char* test_str);

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
	} virtual;
	int total;
	char initialized;
};
struct memory_map* memory_map_get();

void kernel_panic(const char* reason);

#endif /* C75FAF52_8626_4A84_AB39_A02D9C4E44C9 */
