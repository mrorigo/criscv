#ifndef __CSR_H__
#define __CSR_H__

#include <sys/types.h>

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

typedef enum _csr_access_mode_t {
  RW     = 0b01,
  RS     = 0b10,
  RC     = 0b11,
} csr_access_mode_t;

typedef struct _csr_t {
  uint32_t misa;
  uint32_t mvendorid;
  uint32_t marchid;
  uint32_t mimpid;
  uint32_t mhartid;
  uint32_t mstatus;
  uint32_t mtvec;
  uint32_t mie;
  uint32_t mip;
  uint32_t mcause;
  uint32_t mepc;
  uint32_t mscratch;
  uint32_t mtval;
} csr_t;

void csr_init(csr_t *csr);
void csr_cycle(csr_t *csr);

#endif
