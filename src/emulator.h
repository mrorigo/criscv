#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "bus.h"
#include "cpu.h"

typedef struct _emulator_t {
  RV32I_cpu_t *cpu;
  bus_t       *bus;
} emulator_t;

emulator_t *emulator_init();
void emulator_load_elf(emulator_t *emu, const char *filename);
void emulator_run(emulator_t *emu);

#endif
