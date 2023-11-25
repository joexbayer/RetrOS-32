#ifndef __RETROS_KERNEL_H__
#define __RETROS_KERNEL_H__

#include <kutils.h>

#define KERNEL_NAME "RetrOS-32"
#define KERNEL_VERSION "0.0.1"

struct kernel_context {
	struct scheduler* sched_ctx;
	struct windowserver* window_server;
	struct memory_info {
		unsigned int extended_memory_low;
		unsigned int extended_memory_high;
	} *total_memory;
} kernel_context;

#endif /* !__RETROS_KERNEL_H__ */
