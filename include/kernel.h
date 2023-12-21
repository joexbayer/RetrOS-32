#ifndef __RETROS_KERNEL_H__
#define __RETROS_KERNEL_H__

#include <kutils.h>
#include <scheduler.h>
#include <gfx/windowserver.h>

#define KERNEL_NAME		"RetrOS-32"
/* Define the kernel version with compilation date and time */
#define KERNEL_VERSION  "0.0.1 " __DATE__ " " __TIME__
#define KERNEL_RELEASE 	"alpha"

#define KERNEL_ARCH_I386

typedef enum {
	KERNEL_FLAG_NONE = 1 << 0,
	KERNEL_FLAG_TEXTMODE = 1 << 1,
	KERNEL_FLAG_GRAPHICS = 1 << 2,
} graphic_modes_t;

struct kernel_context {
	struct scheduler* sched_ctx;
	struct windowserver* window_server;
	struct allocators {
		void* kernel_heap;
		void* user_heap;
	} allocators;
	struct memory_info {
		unsigned int extended_memory_low;
		unsigned int extended_memory_high;
	} *total_memory;

	graphic_modes_t graphic_mode;
};

struct kernel_context* kernel_get_context();

#endif /* !__RETROS_KERNEL_H__ */
