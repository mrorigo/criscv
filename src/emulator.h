#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "bus.h"
#include "cpu.h"
#include "mmu.h"
#include <pthread.h>

typedef struct _emulator_t {
  RV32I_cpu_t *cpu;
  bus_t       *bus;
  mmu_t       *mmu;
  pthread_t   core_threads[NUMCORES];
  struct _core_thread_args_t core_thread_args[NUMCORES];
  struct _Elf32 *elf;
} emulator_t;

//void		 core_start(RV32I_cpu_t *, uint32_t);
//void		 core_join(RV32I_cpu_t *, uint32_t);

emulator_t *emulator_init();
bool emulator_load_elf(emulator_t *emu, const char *filename);
void emulator_run(emulator_t *emu);

#endif
