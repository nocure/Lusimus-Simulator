#ifndef __MEMORY_FLASH_H__
#define __MEMORY_FLASH_H__
#include "skyeye_defs.h"
int flash_read(short size, uint32_t offset, uint32_t * value);

int flash_write(short size, uint32_t offset, uint32_t value);

#endif
