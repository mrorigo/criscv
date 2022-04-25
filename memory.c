#include <stdlib.h>
#include <sys/types.h>
#include "memory.h"
#include "mmio.h"

void init_ram_rom(mmio_device_t *ram)
{
  ram->user = malloc(ram->size * 4);
}

uint32_t read_ram_rom(const mmio_device_t *ram, const uint32_t offs)
{
  return ram->user[offs & (ram->size-1)];
}

void write_ram_rom(mmio_device_t *ram, const uint32_t offs, const uint32_t value)
{
  ram->user[offs & (ram->size-1)] = value;
}
