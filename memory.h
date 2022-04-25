#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "mmio.h"

void init_ram_rom(mmio_device_t *ram);
uint32_t read_ram_rom(const mmio_device_t *ram, const uint32_t offs);
void write_ram_rom(mmio_device_t *ram, uint32_t offs, uint32_t value);

#endif
