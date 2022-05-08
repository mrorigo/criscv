#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "bus.h"
#include "cpu.h"
#include "mmu.h"

typedef struct _emulator_t {
  RV32I_cpu_t *cpu;
  bus_t       *bus;
  mmu_t       *mmu;
  struct _Elf32 *elf;
} emulator_t;

emulator_t *emulator_init();
bool emulator_load_elf(emulator_t *emu, const char *filename);
void emulator_run(emulator_t *emu);

#endif
