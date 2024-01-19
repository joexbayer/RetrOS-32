#ifndef __KCONF_H__
#define __KCONF_H__

#include <stdint.h>
#include <kernel.h>
#include <kutils.h>
#include <libc.h>

#define CONFIG_MAX_SECTIONS 32
#define CONFIG_MAX_NAME_LEN 32

int kernel_config_load(char* filename);
char* config_get_value(char* section, char* name);
int config_list();


#endif /* __KCONF_H__ */