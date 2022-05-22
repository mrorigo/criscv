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

#include <stdio.h>
#include "csr.h"
#include "config.h"
#include "bus.h"

uint32_t csr_read_write32(csr_t *csr, uint32_t addr, uint32_t val)
{
  const uint32_t ret = csr->state[addr];
  csr->state[addr] = val;
  #ifdef CSR_TRACE
  fprintf(stderr, "csr:read_write32(0x%08x, 0x%08x) => 0x%08x\n", addr, val, ret);
  #endif
  return ret;
}

uint32_t csr_read_set32(csr_t *csr, uint32_t addr, uint32_t mask)
{
  const uint32_t ret = csr->state[addr];
  csr->state[addr] |= mask;
  #ifdef CSR_TRACE
  fprintf(stderr, "csr:read_set32(0x%08x, 0x%08x) => 0x%08x\n", addr, mask, ret);
  #endif
  return ret;
}

uint32_t csr_read_clear32(csr_t *csr, uint32_t addr, uint32_t mask)
{
  const uint32_t ret = csr->state[addr];
  csr->state[addr] &= (~mask);
  #ifdef CSR_TRACE
  fprintf(stderr, "csr:read_clear32(0x%08x, 0x%08x) => 0x%08x\n", addr, mask, ret);
  #endif
  return ret;
}

uint64_t csr_read_write64(csr_t *csr, uint32_t addr, uint64_t val)
{
  const uint64_t ret = csr->state[addr];
  csr->state[addr] = val;
  #ifdef CSR_TRACE
  fprintf(stderr, "csr:read_write64(0x%08x, 0x%08llx) => 0x%016llx\n", addr, val, ret);
  #endif
  return ret;
}

uint64_t csr_read_set64(csr_t *csr, uint32_t addr, uint64_t mask)
{
  const uint64_t ret = csr->state[addr];
  csr->state[addr] |= mask;
  #ifdef CSR_TRACE
  fprintf(stderr, "csr:read_set64(0x%08x, 0x%16llx) => 0x%016llx\n", addr, mask, ret);
  #endif
  return ret;
}

uint64_t csr_read_clear64(csr_t *csr, uint32_t addr, uint64_t mask)
{
  const uint64_t ret = csr->state[addr];
  csr->state[addr] &= (~mask);
  #ifdef CSR_TRACE
  fprintf(stderr, "csr:read_clear64(0x%08x, 0x%16llx) => 0x%016llx\n", addr, mask, ret);
  #endif
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

  // XLEN-1 12 11 10 9 8 7 6 5 4 3 2 1 0
  // WPRI MEIE WPRI SEIE UEIE MTIE WPRI STIE UTIE MSIE WPRI SSIE USIE
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


mmio_device_t csr_mmio_device = {
  .next = NULL,
  .user = NULL,
  .base_address = CSR_MMAP_BASE_ADDR,
  .size = 0x1000,
  .perm = READ|WRITE,
  .init = csr_mmio_init,
  .read_single = csr_mmio_read_single,
  .write_single = csr_mmio_write_single,
};

void csr_mmio_init(mmio_device_t *dev)
{
  dev->state = READY;
}

uint32_t csr_mmio_read_single(mmio_device_t *dev, const uint32_t offs, const memory_access_width_t aw)
{
  (void)offs;
  (void)aw;
  dev->state = ERROR;
  return 0;
}

void csr_mmio_write_single(mmio_device_t *dev, const uint32_t offs, uint32_t value, const memory_access_width_t aw)
{
  (void)offs;
  (void)value;
  (void)aw;
  dev->state = ERROR;
}

uint32_t csr_mmio_read(mmio_device_t *dev, const uint32_t offs, void *buf, const size_t size, const memory_access_width_t aw)
{
  (void)offs;
  (void)buf;
  (void)size;
  (void)aw;
    dev->state = ERROR;
    return 0;
}
uint32_t csr_mmio_write(mmio_device_t *dev, const uint32_t offs, void *buf, const size_t count, const memory_access_width_t aw)
{
  (void)offs;
  (void)buf;
  (void)count;
  (void)aw;
  dev->state = ERROR;
  return 0;
}

