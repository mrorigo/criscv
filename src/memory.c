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
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <assert.h>
#include "mmu.h"
#include "memory.h"
#include "mmio.h"
#include "config.h"

void init_ram(mmio_device_t *ram)
{
  // Nothing much to do here..
}

__attribute((__always_inline__))
  uint32_t read_ram(const mmio_device_t *ram, const vaddr_t offs, const memory_access_width_t aw)
{
  mmu_t *mmu = (mmu_t *)ram->user;

  if(aw == WORD) {
    uint32_t ret = 0;
    mmu_read_into(mmu, &ret, offs, sizeof(uint32_t));
    return ret;
  } else if(aw == HALFWORD) {
    uint16_t ret = 0;
    mmu_read_into(mmu, &ret, offs, sizeof(uint16_t));
    return ret;
  } else if(aw == BYTE) {
    uint8_t ret = 0;
    mmu_read_into(mmu, &ret, offs, sizeof(uint8_t));
    return ret;
  }
  assert(0==1);
}

__attribute((__always_inline__))
void write_ram(mmio_device_t *ram,
	       const vaddr_t offs,
	       const uint32_t value,
	       const memory_access_width_t aw)
{
  mmu_t *mmu = (mmu_t *)ram->user;
  //  fprintf(stderr, "memory::write_ram: %08x = %08x (%u)\n", offs, value, aw);
  if(aw == WORD) {
    assert((offs & 3) == 0);
    mmu_write_from(mmu, (void*)&value, offs, sizeof(uint32_t));
  } else if(aw == HALFWORD) {
    assert((offs & 1) == 0);
    uint16_t v16 = (uint32_t)value&0xffff;
    mmu_write_from(mmu, (void*)&v16, offs, sizeof(uint16_t));
  } else if(aw == BYTE) {
    uint8_t v8 = (uint32_t)value&0xff;
    mmu_write_from(mmu, (void*)&v8, offs, sizeof(uint8_t));
  }
}
