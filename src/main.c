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
#include "mmio.h"
#include "bus.h"
#include "memory.h"
#include "csr.h"
#include "cpu.h"
#include "elf32.h"


#define RAM_SIZE 1024 * 1000 * 4 // 4mb
#define ROM_SIZE 1024 * 1000     // 1mb


#define ROM_START 0x10000000
#define RAM_START 0x20000000

#define STACK_SIZE (1<<12)  // 16kb is enough for everyone

#define STACK_TOP ((RAM_START + RAM_SIZE) - (STACK_SIZE))


mmio_device_t rom_device = {
  .next		= NULL,
  .base_address = ROM_START,
  .size		= ROM_SIZE,
  .init		= init_ram_rom,
  .read		= read_ram_rom,
  .write	= write_ram_rom
};

mmio_device_t ram_device = {
  .next         = &rom_device,
  .base_address = RAM_START,
  .size		= RAM_SIZE,
  .init		= init_ram_rom,
  .read		= read_ram_rom,
  .write	= write_ram_rom
};

bus_t main_bus = {
  .mmio_devices = &ram_device // RAM first, most accessed
};

#define se_12_32(x) (((uint32_t)x << 20) >> 20)
#define mask12(x) ((x) & ((1<<12)-1))

#define I_INSTRUCTION(opcode, imm12, rs1, rd)  ((opcode&0x7f) | (mask12(se_12_32(imm12)) << 20) | (rs1 << 15) | (rd<<7) | ((opcode >> 7) & 0b111)<<12)

#define U_INSTRUCTION(opcode, imm20, rd)  ((opcode&0x7f)  | (rd << 7) | (imm20<<12))
#define S_INSTRUCTION(opcode, imm20, rs1, rs2)  ((opcode&0x7f)  | (rs1 << 15) | (rs2 << 20) |   (imm20&31) | ((imm20&0xfffe0) << 25))


static const uint32_t rom[9] = {
    // ra=Return address == HERE
    U_INSTRUCTION(OP_LUI, (ROM_START >> 12), 1), // LUI X1, ROM_START>>12

    // sp=Initial stack
    U_INSTRUCTION(OP_LUI, (STACK_TOP>>12), 2), // LUI X2, STACK_TOP>>12
    I_INSTRUCTION(OP_ADDI, 0xffc, 2, 2), // TODO: Why not addi works!?

    // gp=Global pointer = RAM
    U_INSTRUCTION(OP_LUI, (RAM_START >> 12), 3), // LUI X1, ROM_START>>12

    // tp=Thread pointer = RAM
    U_INSTRUCTION(OP_LUI, (RAM_START >> 12), 4), // LUI X1, ROM_START>>12

    // Load the entry address from RAM
    I_INSTRUCTION(OP_LW,   0, 2, 31),                   // LW x31 => X2[0]
    
    // Jump to _start
    I_INSTRUCTION(OP_JALR, 0, 31, 30),                   // JALR x31,30,x30

    //  STYPE_INSTRUCTION(OP_SB, 3, 1, 2),                 // STORE 42 at X1+3 (RAM_START+3)
  //  UTYPE_INSTRUCTION(OP_AUIPC, 0xffc, 3),
  //  ITYPE_INSTRUCTION(OP_ADDI, -1, 0, 2),
  //  ITYPE_INSTRUCTION(OP_SLTI, 0,  2, 1),
  0xdeadbeef,
  0x0badc0de
};

//0x203dfffc
//0x203deffc    
void load_initial_rom(bus_t *bus, const uint32_t base_addr, const uint32_t *rom, size_t size_in_bytes)
{
  assert(size_in_bytes < ROM_SIZE);
  for(size_t i=0; i < size_in_bytes>>2; i++) {
    bus_write_single(bus, base_addr + i*sizeof(uint32_t), rom[i], WORD);
  }
}



int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  FILE *fd = fopen(argv[1], "rb");
  assert(fd);
  assert(fseek(fd, 0, SEEK_END) == 0);
  uint32_t size = ftello(fd);
  assert(size > 0);
  fprintf(stderr, "read elf file %s of size %d\n", argv[1], size);
  void *elf = malloc(size);
  assert(fseek(fd, 0, SEEK_SET) == 0);
  assert(fread(elf, size, 1, fd) == 1);
  fclose(fd);

  
  fprintf(stderr, "initializing main bus\n");
  bus_init(&main_bus);

  
  fprintf(stderr, "loading initial ROM @ %08x\n", ROM_START);

  uint32_t entry_point = (uint32_t)(elf_load(elf, size));

  fprintf(stderr, "entry point from elf file: 0x%08x, stack_top=0x%08x size=%d\n", entry_point, STACK_TOP, STACK_SIZE);
  assert(entry_point);
  entry_point += RAM_START-0x00010000; // riscv-toolchain uses this base addr
  load_initial_rom(&main_bus, ROM_START, rom, sizeof(rom));
  load_initial_rom(&main_bus, RAM_START, elf, size);

  // store entry point above stack
  bus_write_single(&main_bus, STACK_TOP - 4, entry_point, WORD);

  
  fprintf(stderr, "initializing CPU\n");
  RV32I_cpu_t *cpu = cpu_init((uint32_t)ROM_START, &main_bus);

  
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  uint64_t lastc = 0;
  uint64_t start = spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
  while(true) {
    if(cpu->cores[0].halted) {
      fprintf(stderr, "CPU:CORE0: HALT\n");
      break;
    }
    cpu_cycle(cpu, 0);
    if(cpu->cores[0].cycle % 100000000 == 0) {
      struct timespec spec;
      uint64_t cycles = cpu->cores[0].cycle / 5;

      clock_gettime(CLOCK_REALTIME, &spec);
      uint64_t end = spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
      float mips = (float)(cycles-lastc) / ((float)(end-start));
      fprintf(stderr, "mip/s: %2f\n", (mips/1e3));
      start = end;
      lastc = cycles;
    }
  }
}
