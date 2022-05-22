#ifndef __EMULATOR_H__
#define __EMULATOR_H__

#include "bus.h"
#include "cpu.h"
#include "mmu.h"
#include "video.h"
#include <pthread.h>

typedef struct _emulator_t {
  RV32I_cpu_t *cpu;
  bus_t       *bus;
  mmu_t       *mmu;
  video_t     video;
  pthread_t   core_threads[NUMCORES];
  struct _core_thread_args_t core_thread_args[NUMCORES];
  struct _Elf32 *elf;
} emulator_t;

emulator_t *emulator_init();
bool emulator_load_elf(emulator_t *emu, const char *filename);
void emulator_run(emulator_t *emu, const char *argv1);

#endif
