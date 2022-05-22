#ifndef __CSR_H__
#define __CSR_H__

#include <sys/types.h>
#include "mmio.h"

typedef struct _trap_args_t {
  struct _core_t *core;
  struct _emulator_t *emulator;
} trap_args_t;

typedef enum _csr_instruction_type_t {
  CSRRW = 0b001,
  CSRRS = 0b010,
  CSRRC = 0b011,
  CSRRWI = 0b101,
  CSRRSI = 0b110,
  CSRRCI = 0b111
} csr_instruction_type_t;

typedef enum _csr_mmode_t {
  mstatus   =	0x300,
  misa      =	0x301,
  mie       =	0x304,
  mtvec     =	0x305,
  mscratch  =	0x340,
  mepc      =	0x341,
  mcause    =	0x342,
  mtval     =	0x343,
  mip       =	0x344,
  mvendorid =	0xF11,
  marchid   =	0xF12,
  mimpid    =	0xF13,
  mhartid   =	0xF14,

  csr_mmode_max = 0x1000
} csr_mmode_t;

#define TRAP_CAUSE_INTERRUPT (1<<5)
typedef enum _trap_cause_t {
  SSI = 1 | TRAP_CAUSE_INTERRUPT,  // Supervisor Software Interrupt
  MSI = 3 | TRAP_CAUSE_INTERRUPT,  // Nachine Software Interrupt
  STI = 5 | TRAP_CAUSE_INTERRUPT,  // Supervisor Timer Interrupt
  MTI = 7 | TRAP_CAUSE_INTERRUPT,  // Machine Timer Interrupt
  SEI = 9 | TRAP_CAUSE_INTERRUPT,  // Supervisor External Interrupt

  INSTRUCTION_ADDR_MISALIGN = 0,
  INSTRUCTION_ACCESS_FAULT = 1,
  ILLEGAL_INSTRUCTION = 2,
  BREAKPOINT = 3,
  LOAD_ADDR_MISALIGNED = 4,
  LOAD_ACCESS_FAULT = 5,
  STORE_ADDR_MISALIGNED = 6,
  STORE_ACCESS_FAULT = 7,
  ENV_CALL_UMODE = 8,
  ENV_CALL_SMODE = 9,
  ENV_CALL_MMODE = 11,
  INSTRUCTION_PAGE_FAULT = 12,
  LOAD_PAGE_FAULT = 13,
  STORE_PAGE_FAULT = 15
} trap_cause_t;

typedef enum _csr_access_mode_t {
  RW     = 0b01,
  RS     = 0b10,
  RC     = 0b11,
} csr_access_mode_t;

typedef struct _csr_t {
  uint32_t      state[csr_mmode_max];
} csr_t;

void csr_init(csr_t *csr);
uint32_t csr_read_write(csr_t *csr, uint32_t addr, uint32_t val);
uint32_t csr_read_set(csr_t *csr, uint32_t addr, uint32_t mask);
uint32_t csr_read_clear(csr_t *csr, uint32_t addr);

#endif
