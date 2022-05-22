
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "config.h"
#include "emulator.h"
#include "memory.h"
#include "elf32.h"
#include "syscall.h"
#include "video.h"
#include "csr.h"

mmio_device_t ram_device = {
  .next         = &csr_mmio_device,
  .base_address = RAM_START,
  .size		= RAM_SIZE,
  .state        = READY,
  .perm         = READ|WRITE,
  .init		= init_ram,
  .read         = read_ram,
  .read_single	= read_ram_single,
  .write         = write_ram,
  .write_single	= write_ram_single
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

  if(!video_init(&emu->video, emu->mmu)) {
    free(emu);
    return NULL;
  }

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



bool handle_umode_call(emulator_t *emu, core_t *core)
{
  const uint32_t syscall_num = core->registers[17]; // a7
  const uint32_t arg0 = core->registers[10]; // a0
  const uint32_t arg1 = core->registers[11]; // a1
  const uint32_t arg2 = core->registers[12]; // a2
  const uint32_t arg3 = core->registers[13]; // a3
  const uint32_t arg4 = core->registers[14]; // a4
  //  core_dumpregs(core);
  return handle_syscall(emu, core, syscall_num, arg0, arg1, arg2, arg3, arg4);
}


bool trap_handler(core_thread_args_t *args)
{
  core_t *core = args->core;
  emulator_t *emu = args->emulator;

  //  fprintf(stderr, "emu::trap_handler::TRAP at 0x%08x: cause=0x%02x \n", core->csr.mepc,  core->csr.mcause);
  trap_cause_t cause = csr_read_clear32(&core->csr, mcause, 0);
  vaddr_t trap_pc = csr_read_clear32(&core->csr, mepc, 0);
  switch(cause) {
  case ENV_CALL_UMODE:
    return handle_umode_call(emu, core);

  case ILLEGAL_INSTRUCTION:
    fprintf(stderr, "emu::trap_handler::ILLEGAL_INSTRUCTION @ 0x%08x \n", trap_pc);
    return false;
  case LOAD_ADDR_MISALIGNED:
    fprintf(stderr, "emu::trap_handler::LOAD_ADDR_MISALIGNED @ 0x%08x \n", trap_pc);
    return false;
  case STORE_ADDR_MISALIGNED:
    fprintf(stderr, "emu::trap_handler::STORE_ADDR_MISALIGNED @ 0x%08x \n", trap_pc);
    return false;
  case INSTRUCTION_ADDR_MISALIGN:
    fprintf(stderr, "emu::trap_handler::INSTRUCTION_ADDR_MISALIGN @ 0x%08x \n", trap_pc);
    return false;
  case INSTRUCTION_ACCESS_FAULT:
    fprintf(stderr, "emu::trap_handler::INSTRUCTION_ACCESS_FAULT @ 0x%08x \n", trap_pc);
    return false;
  case LOAD_ACCESS_FAULT:
    fprintf(stderr, "emu::trap_handler::LOAD_ACCESS_FAULT @ 0x%08x \n", trap_pc);
    return false;

  case STORE_ACCESS_FAULT:
    fprintf(stderr, "emu::trap_handler::STORE_ACCESS_FAULT @ 0x%08x \n", trap_pc);
    return false;

  default:
    fprintf(stderr, "emu::trap_handler::UNKNOWN TRAP @ 0x%08x: cause=0x%02x \n", trap_pc,  cause);
    return false;
    
  }
  return false;
}

void * cpu_thread(void *arg)
{
  core_thread_args_t *args = (core_thread_args_t *)arg;
  //  emulator_t *emu = args->emulator;
  core_t *core = args->core;
  assert(core);

  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  uint64_t lastc = 0;
  uint64_t start = spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
  while(true) {
    core_cycle(core);
    if(core->state == TRAP && core->trap_state == HANDLE) {
      // Call from usermode (ECALL);
      if(core->trap_handler != NULL) {
	if(!core->trap_handler(args)) {
	  fprintf(stderr, "cpu core: trap_handler returned false, core exiting\n");
	  break; // stop cpu loop
	}
      } else {
	  fprintf(stderr, "cpu core: trap_handler NOT defined, exiting\n");
	  break;
      }
    }

    if(core->cycle % (uint64_t)1e8 == 0) {
      struct timespec spec;
      uint64_t cycles = core->cycle / 5;

      clock_gettime(CLOCK_REALTIME, &spec);
      uint64_t end = spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
      float mips = (float)(cycles-lastc) / ((float)(end-start));
      fprintf(stderr, "mip/s: %2f\n", (mips/1e3));
      start = end;
      lastc = cycles;
    }
  }
  return NULL;
}


void core_start(emulator_t *emu, uint32_t core_num)
{
  core_thread_args_t *args = &emu->core_thread_args[core_num];
  args->emulator = emu;
  args->core = &emu->cpu->cores[core_num];
  pthread_create(&emu->core_threads[core_num],
		 NULL,
		 cpu_thread,
		 args);
}

void core_join(emulator_t *emu, uint32_t core_num)
{
  pthread_join(emu->core_threads[core_num], NULL);
}


void emulator_run(emulator_t *emu, const char *argv1)
{
  fprintf(stderr, "initializing CPU\n");
  emu->cpu = cpu_init(emu->bus);

  fprintf(stderr, "Initializing %d cores with pc 0x%08x\n", NUMCORES, emu->elf->entry);
  for(size_t i=0; i < NUMCORES; i++) {
    core_t *core = &emu->cpu->cores[i];
    const vaddr_t stack = mmu_allocate_raw(emu->mmu, STACK_SIZE);
    const vaddr_t stack_top = stack + STACK_SIZE;
    fprintf(stderr, "Stack allocated at 0x%08x, top at 0%08x\n", stack, stack_top);
    core_init(emu->cpu, i, emu->elf->entry);
    // return address lives in x1
    core->registers[1] = 0x1337c0de;//emu->elf->entry;

    // Allocate argv[1]
    const char *argv0 = "./program_name\0";
    vaddr_t av0 = mmu_allocate(emu->mmu, strlen(argv0)+1, MPERM_RAW|MPERM_WRITE);
    bus_write_multiple(emu->bus, av0, (void*)argv0, strlen(argv0)+1, BYTE);
    //    const char *argv1 = "this_is_argv1\0";
    vaddr_t av1 = mmu_allocate(emu->mmu, strlen(argv1)+1, MPERM_RAW|MPERM_WRITE);
    bus_write_multiple(emu->bus, av1, (void*)argv1, strlen(argv1)+1, BYTE);

    // stack lives in x2
    core->registers[2] = stack_top;
    core->trap_handler = trap_handler;
#define push(x) { vaddr_t sp = core->registers[X2] - sizeof(uint32_t); bus_write_single(emu->bus, sp, x, WORD); core->registers[X2] = sp; }
    push(0); // auxp
    push(0); // envp
    push(0); // argv null
    push(av1); // argv[1] null
    push(av0); // argv[0]
    // add argv[..]
    push(42); // argc 
#undef push    
    
    core_start(emu, i);
  }

  for(size_t i=0; i < NUMCORES; i++) {
    core_join(emu, i);
  }
}
