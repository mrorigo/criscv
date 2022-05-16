
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "config.h"
#include "emulator.h"
#include "memory.h"
#include "elf32.h"


mmio_device_t ram_device = {
  .next         = NULL,
  .base_address = RAM_START,
  .size		= RAM_SIZE,
  .perm         = READ|WRITE|EXEC,
  .init		= init_ram,
  .read		= read_ram,
  .write	= write_ram
};

bus_t main_bus = {
  .mmio_devices = &ram_device // RAM first, most accessed
};


void load_into_ram(bus_t *bus,
		   const uint32_t base_addr,
		   const uint32_t *rom,
		   size_t size_in_bytes)
{
  for(size_t i=0; i < size_in_bytes>>2; i++) {
    bus_write_single(bus, base_addr + i*sizeof(uint32_t), rom[i], WORD);
  }
}


uint8_t *load_file(const char *filename, size_t *osize)
{
  *osize = 0;

  FILE *fd = fopen(filename, "rb");
  if(!fd) {
    fprintf(stderr, "ERROR: Could not load ELF file %s\n", filename);
    return NULL;
  }
  assert(fd);
  assert(fseek(fd, 0, SEEK_END) == 0);
  uint32_t size = ftello(fd);
  assert(size > 0);
  fprintf(stderr, "load_file: read %s, size %d\n", filename, size);

  void *ptr = malloc(size);
  if(!ptr) {
    return NULL;
  }
  assert(fseek(fd, 0, SEEK_SET) == 0);
  assert(fread(ptr, size, 1, fd) == 1);
  fclose(fd);

  *osize = size;
  return (uint8_t *)ptr;
}

emulator_t *emulator_init()
{
  emulator_t *emu = malloc(sizeof(emulator_t));
  assert(emu);
  emu->bus = &main_bus;
  emu->mmu = mmu_init(RAM_START, RAM_SIZE);
  ram_device.user = (void *)emu->mmu;

  // Allocate space for ISR ptr
  //  vaddr_t isr_addr = mmu_allocate_raw(emu->mmu, sizeof(vaddr_t)*64);
  //  assert(isr_addr == RAM_START);


  bus_init(emu->bus);
  //  bus_write_single(emu->bus, isr_addr,   0x00001337, WORD);
  
  fprintf(stderr, "initializing main bus\n");
  return emu;
}

bool emulator_load_elf(emulator_t *emu, const char *filename)
{
  size_t elf_size;
  fprintf(stderr, "Load ELF file %s into RAM\n", filename);
  uint8_t *file = load_file(filename, &elf_size);
  if(!file) {
    return false;
  }
  emu->elf = elf_load(file, emu->mmu);
  fprintf(stderr, "- entry point: 0x%08x\n", emu->elf->entry);
  assert(emu->elf->entry);
  free(file);

  return true;
}


void emulator_run(emulator_t *emu)
{
  fprintf(stderr, "initializing CPU\n");
  RV32I_cpu_t *cpu = cpu_init(emu->bus, NUMCORES);

  fprintf(stderr, "Initializing %d cores with pc 0x%08x\n", NUMCORES, emu->elf->entry);
  for(size_t i=0; i < NUMCORES; i++) {
    const vaddr_t stack = mmu_allocate_raw(emu->mmu, STACK_SIZE);
    const vaddr_t stack_top = stack + STACK_SIZE - sizeof(uint32_t)*2;
    fprintf(stderr, "Stack allocated at 0x%08x, top at 0%08x\n", stack, stack_top);
    core_init(cpu, i, emu->elf->entry);
    // return address lives in x1
    cpu->cores[i]->registers[1] = 0x1337c0de;//emu->elf->entry;

    // stack lives in x2
    cpu->cores[i]->registers[2] = stack_top;

#define push(x) { vaddr_t sp = cpu->cores[i]->registers[X2] - sizeof(vaddr_t); bus_write_single(emu->bus, sp, 0, WORD); cpu->cores[i]->registers[X2] = sp; }

    push(0); // auxp
    push(0); // envp
    push(0); // argv null
    // add args
    push(0); // argc 
#undef push    
    
    core_start(cpu, i);
  }

  for(size_t i=0; i < NUMCORES; i++) {
    core_join(cpu, i);
  }
}
