/**

Copyright 2022 orIgo <mrorigo@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <math.h>
#include "config.h"
#include "mmio.h"
#include "bus.h"
#include "memory.h"
#include "csr.h"
#include "cpu.h"
#include "elf32.h"

// Default mmio devices
mmio_device_t rom_device = {
  .next		= NULL,
  .base_address = ROM_START,
  .size		= ROM_SIZE,
  .perm         = READ|EXEC,
  .init		= init_ram_rom,
  .read		= read_ram_rom,
  .write	= write_ram_rom
};

mmio_device_t ram_device = {
  .next         = &rom_device,
  .base_address = RAM_START,
  .size		= RAM_SIZE,
  .perm         = READ|WRITE|EXEC,
  .init		= init_ram_rom,
  .read		= read_ram_rom,
  .write	= write_ram_rom
};

bus_t main_bus = {
  .mmio_devices = &ram_device // RAM first, most accessed
};

// Initial ROM sets up stack ptr, reads STACK_TOP-4 for entry point, and jumps there
uint32_t rom[] = {
  /*0:*/	0x100000b7, //         	lui	x1,0x10000
  /*4:*/	0x203e7137, //         	lui	x2,0x203e7
  /*8:*/	0xffc10113, //         	addi	x2,x2,-4 # 203e6ffc
  /*c:*/	0x200001b7, //         	lui	x3,0x20000
  /*10:*/	0x00018213, //         	addi	x4,x3,0 # 20000000
  /*14:*/	0x00012f83, //         	lw	x31,0(x2)
  /*18:*/	0x000f8067, //         	jalr	x0,0(x31)
};

const size_t rom_size = sizeof(rom);


//0x203dfffc
//0x203deffc    
void load_initial_rom(bus_t *bus, const uint32_t base_addr, const uint32_t *rom, size_t size_in_bytes)
{
  for(size_t i=0; i < size_in_bytes>>2; i++) {
    bus_write_single(bus, base_addr + i*sizeof(uint32_t), rom[i], WORD);
  }
}

uint32_t *load_file(const char *filename, size_t *osize)
{
  *osize = 0;

  FILE *fd = fopen(filename, "rb");
  assert(fd);
  assert(fseek(fd, 0, SEEK_END) == 0);
  uint32_t size = ftello(fd);
  assert(size > 0);
  fprintf(stderr, "load_file: read %s, size %d\n", filename, size);

  void *ptr = malloc(size);
  assert(fseek(fd, 0, SEEK_SET) == 0);
  assert(fread(ptr, size, 1, fd) == 1);
  fclose(fd);

  *osize = size;
  return (uint32_t *)ptr;
}

int main(int argc, char **argv)
{
  (void)argc;

  size_t elf_size;
  
  fprintf(stderr, "initializing main bus\n");
  bus_init(&main_bus);

  fprintf(stderr, "Load ROM (%d)\n", rom_size);
  //  uint32_t *rom = load_file(argv[1], &rom_size);
  rom_device.perm = READ|WRITE|EXEC;
  load_initial_rom(&main_bus, ROM_START, rom, rom_size);
  rom_device.perm = READ|EXEC;

  fprintf(stderr, "Load ELF file %s into RAM\n", argv[1]);
  uint32_t *elf = load_file(argv[1], &elf_size);
  uint32_t entry_point = (uint32_t)(elf_load(elf, elf_size));
  fprintf(stderr, "- entry point: 0x%08x, stack_top=0x%08x stack_size=%d\n", entry_point, STACK_TOP, STACK_SIZE);
  assert(entry_point);

  // Load the ELF into RAM (+4, as mtvec is first)
  load_initial_rom(&main_bus, RAM_START+4, elf, elf_size);
  free(elf); elf = NULL;
  
  // store entry point on stack top for the ROM to read
  entry_point += RAM_START+4-0x00010000; // riscv64-toolchain uses this base addr
  bus_write_single(&main_bus, STACK_TOP - sizeof(uint32_t), entry_point, WORD);

  // store ISR entry point at RAM_START
  bus_write_single(&main_bus, RAM_START, 0x00001337, WORD);

  fprintf(stderr, "initializing CPU\n");
  RV32I_cpu_t *cpu = cpu_init(&main_bus, NUMCORES);

  fprintf(stderr, "Initializing %d cores\n", NUMCORES);
  for(size_t i=0; i < NUMCORES; i++) {
    core_init(cpu, i, ROM_START);
    core_start(cpu, i);
  }

  for(size_t i=0; i < NUMCORES; i++) {
    core_join(cpu, i);
  }
}
