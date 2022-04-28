
#include "csr.h"


void csr_init(csr_t *csr)
{
  csr->misa = 0x40000100;
  // Encodes CPU capabilities, top 2 bits encode width (XLEN), bottom 26 encode extensions
  csr->misa      = 0x40000100;
  // JEDEC manufacturer ID
  csr->mvendorid = 0;
  // Microarchitecture ID
  csr->marchid   = 0;
  // Processor version
  csr->mimpid    = 0;
  // Hart ID
  csr->mhartid   = 0;
  // Various specific flags and settings, including global interrupt enable, and a lot of noop bits
  csr->mstatus   = 0;
  // Encodes the base trap vector address + mode (table or single handler)
  csr->mtvec     = 0x10000004 | 1; // FIXME
  // Interrupt enable / disable
  csr->mie       = 0x00000888;
  // Interrupt-pending
  csr->mip       = 0;
  // Trap cause. Top bit set = interrupt, reset = exception - reset indicates the type
  csr->mcause    = 0;
  // Exception Program Counter
  csr->mepc      = 0;
  // General use reg for M-Mode
  csr->mscratch  = 0;
  // Trap-value register, can hold the address of a faulting instruction
  csr->mtval     = 0;
}
