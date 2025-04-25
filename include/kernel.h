#ifndef __RETROS_KERNEL_H__
#define __RETROS_KERNEL_H__

#include <kutils.h>
#include <scheduler.h>
#include <gfx/windowserver.h>
#include <usermanager.h>
#include <net/networkmanager.h>
#include <kevents.h>
#include <gfx/core.h>

#define KERNEL_NAME		"RetrOS-32"
/* Define the kernel version with compilation date and time */
#define KERNEL_VERSION  "0.0.5 "
#define KERNEL_RELEASE 	"alpha"
#define KERNEL_DATE		__DATE__
#define KERNEL_TIME		__TIME__

#define KERNEL_ARCH_I386

typedef enum {
	KERNEL_FLAG_NONE = 1 << 0,
	KERNEL_FLAG_TEXTMODE = 1 << 1,
	KERNEL_FLAG_GRAPHICS = 1 << 2,
} graphic_modes_t;

struct kernel_context {

	struct kernel_services {
		struct usermanager* usermanager;
		struct scheduler* scheduler;
		struct networkmanager* networking;
		struct kevents* kevents;
	} services;

	struct graphics {
		struct graphic_context* ctx;
		struct windowserver* window_server;
	} graphics;

	struct allocators {
		void* kernel_heap;
		void* user_heap;
	} allocators;
	
	struct boot_info {
		unsigned int extended_memory_low;
		unsigned int extended_memory_high;
		unsigned int textmode;
	} *boot_info;

	graphic_modes_t graphic_mode;
};
extern struct kernel_context* $kernel;
extern struct kernel_services* $services;

struct kernel_context* kernel_get_context();


#endif /* !__RETROS_KERNEL_H__ */
