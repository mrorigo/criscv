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

#define se_12_32(x) (((uint32_t)x << 20) >> 20)
#define mask12(x) ((x) & ((1<<12)-1))

#define ITYPE_INSTRUCTION(opcode, imm12, rs1, rd)  ((opcode&0x7f) | (mask12(se_12_32(imm12)) << 20) | (rs1 << 15) | (rd<<7) | ((opcode >> 7) & 0b111)<<12)
//#define ITYPE_INSTRUCTION(opcode, imm12, rs1, rd)  (opcode)

#define UTYPE_INSTRUCTION(opcode, imm12, rd)  ((opcode&0x7f)  | (rd << 7) | (mask12(imm12)<<12))

static const uint32_t rom[5] = {
  UTYPE_INSTRUCTION(OP_LUI, 0xfff, 2),
  ITYPE_INSTRUCTION(OP_ADDI, -1, 0, 2),
  ITYPE_INSTRUCTION(OP_SLTI, 0,  2, 1),
  0xdeadbeef,
  0x0badc0de
};

void load_initial_rom(bus_t *bus, const uint32_t *rom, size_t size_in_bytes)
{
  assert(size_in_bytes < ROM_SIZE);
  for(size_t i=0; i < size_in_bytes>>2; i++) {
    bus_write(bus, ROM_START + i*sizeof(uint32_t), rom[i]);
  }
}



int main(int argc, char **argv)
{
  fprintf(stderr, "initializing main bus\n");
  bus_init(&main_bus);

  fprintf(stderr, "loading initial ROM: %08lx\n",
	  rom[0]);
  load_initial_rom(&main_bus, rom, sizeof(rom));

  fprintf(stderr, "initializing CPU\n");
  RV32I_t *cpu = cpu_init(ROM_START, &main_bus);

  while(true) {
    if(cpu->cores[0].halted) {
      fprintf(stderr, "CPU:CORE0: HALT\n");
      break;
    }
    cpu_cycle(cpu, 0);
  }
};
