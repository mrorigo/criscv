#ifndef __CSR_H__
#define __CSR_H__

#include <sys/types.h>

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
  misa      =	0x301,
  mvendorid =	0xF11,
  marchid   =	0xF12,
  mimpid    =	0xF13,
  mhartid   =	0xF14,
  mstatus   =	0x300,
  mtvec     =	0x305,
  mie       =	0x304,
  mip       =	0x344,
  mcause    =	0x342,
  mepc      =	0x341,
  mscratch  =	0x340,
  mtval     =	0x343,
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
  uint32_t	misa;
  uint32_t	mvendorid;
  uint32_t	marchid;
  uint32_t	mimpid;
  uint32_t	mhartid;
  uint32_t	mstatus;
  uint32_t	mtvec;
  uint32_t	mie;
  uint32_t	mip;
  trap_cause_t	mcause;
  uint32_t	mepc;
  uint32_t	mscratch;
  uint32_t	mtval;
} csr_t;

void csr_init(csr_t *csr);
void csr_cycle(csr_t *csr);

#endif
