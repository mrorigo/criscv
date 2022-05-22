/**
   Copyright 2022 orIgo <mrorigo@gmail.com>

   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
   the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
   FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
   COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "csr.h"
#include "config.h"
#include "bus.h"

uint32_t csr_read_write(csr_t *csr, uint32_t addr, uint32_t val)
{
  const uint32_t ret = csr->state[addr];
  csr->state[addr] = val;
  return ret;
}

uint32_t csr_read_set(csr_t *csr, uint32_t addr, uint32_t mask)
{
  const uint32_t ret = csr->state[addr];
  csr->state[addr] |= mask;
  return ret;
}

uint32_t csr_read_clear(csr_t *csr, uint32_t addr)
{
  const uint32_t ret = csr->state[addr];
  csr->state[addr] = 0;
  return ret;
}

void csr_init(csr_t *csr)
{
  // Encodes CPU capabilities, top 2 bits encode width (XLEN), bottom 26 encode extensions
  csr->state[misa]      = 0x40000100;
  // JEDEC manufacturer ID
  csr->state[mvendorid] = 0x1337;
  // Microarchitecture ID
  csr->state[marchid]   = 0x6502;
  // Processor version
  csr->state[mimpid]    = 0x777;
  // Hart ID
  csr->state[mhartid]   = 0;
  // Various specific flags and settings, including global interrupt enable, and a lot of noop bits
  csr->state[mstatus]   = 0;
  // Encodes the base trap vector address + mode (table or single handler)
  csr->state[mtvec]     = RAM_START-4; // single trap handler, start of RAM-4
                   
  // Interrupt enable / disable
  csr->state[mie]       = 0x00000888;
  // Interrupt-pending
  csr->state[mip]       = 0;
  // Trap cause. Top bit set = interrupt, reset = exception - reset indicates the type
  csr->state[mcause]    = 0;
  // Exception Program Counter
  csr->state[mepc]      = 0;
  // General use reg for M-Mode
  csr->state[mscratch]  = 0;
  // Trap-value register, can hold the address of a faulting instruction
  csr->state[mtval]     = 0;
}

