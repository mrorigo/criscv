#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "mmio.h"
#include "bus.h"
#include "memory.h"
#include "cpu.h"


#define RAM_SIZE 1024 * 1000 * 4 // 4mb
#define ROM_SIZE 1024 * 1000     // 1mb

#define ROM_START 0x10000000
#define RAM_START 0x20000000

mmio_device_t ram_device = {
  .next         = NULL,
  .base_address = RAM_START,
  .size		= RAM_SIZE,
  .init		= init_ram_rom,
  .read		= read_ram_rom,
  .write	= write_ram_rom
};

mmio_device_t rom_device = {
  .next		= &ram_device,
  .base_address = ROM_START,
  .size		= ROM_SIZE,
  .init		= init_ram_rom,
  .read		= read_ram_rom,
  .write	= write_ram_rom
};

bus_t main_bus = {
  .mmio_devices = &rom_device
};

static const uint32_t rom[1] = {
  0xdeadbeef
};

void load_initial_rom(bus_t *bus, uint32_t *rom, size_t size_in_bytes)
{
  assert(size_in_bytes < ROM_SIZE);
  for(size_t i=0; i < size_in_bytes>>2; i++) {
    bus_write(bus, i, rom[i]);
  }
}

int main(int argc, char **argv)
{
  fprintf(stderr, "initializing main bus\n");
  bus_init(&main_bus);

  fprintf(stderr, "loading initial ROM\n");
  load_initial_rom(&main_bus, rom, sizeof(rom)/4);

  fprintf(stderr, "initializing CPU\n");
  RV32I_t *cpu = cpu_init(rom_device.base_address, &main_bus);

  while(true) {
    if(cpu->cores[0].halted) {
      fprintf(stderr, "CPU:CORE0: HALT\n");
      break;
    }
    cpu_cycle(cpu, 0);
  }
};
