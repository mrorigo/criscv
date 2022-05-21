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
  ram->state = READY;
  // Nothing much to do here..
}

size_t write_ram(struct _mmio_device_t *ram,
		 const uint32_t offs,
		 const void *buf,
		 size_t count,
		 const memory_access_width_t aw)
{
  mmu_t *mmu = (mmu_t *)ram->user;
  ram->state = BUSY;

  const size_t mult = aw == WORD ? 4 : (aw == HALFWORD ? 2 : 1);
  const size_t ret = mmu_write_from(mmu, buf, offs, count*mult);

  if(mmu->state == MMU_OK) {
    ram->state = READY;
    return ret;
  }
  ram->state = ERROR;
  return 0;
}

size_t read_ram(struct _mmio_device_t *ram, const uint32_t offs, void *buf, size_t count, const memory_access_width_t aw)
{
  mmu_t *mmu = (mmu_t *)ram->user;
  ram->state = BUSY;
  const size_t mult = aw == WORD ? 4 : (aw == HALFWORD ? 2 : 1);
  const size_t ret = mmu_read_into(mmu, buf, offs, count*mult);
  if(mmu->state == MMU_OK) {
    ram->state = READY;
    return ret;
  }
  ram->state = ERROR;
  return 0;
}

__attribute((__always_inline__))
  uint32_t read_ram_single(mmio_device_t *ram,
			   const vaddr_t offs,
			   const memory_access_width_t aw)
{
  if(aw == WORD) {
    uint32_t ret = 0;
    read_ram(ram, offs, &ret, 1, aw);
    return ret;
  } else if(aw == HALFWORD) {
    uint32_t ret = 0;
    read_ram(ram, offs, &ret, 1, aw);
    return ret;
  } else if(aw == BYTE) {
    uint8_t ret;
    read_ram(ram, offs, &ret, 1, aw);
    return ret;
  }
  assert(false);
}

__attribute((__always_inline__))
  void write_ram_single(mmio_device_t *ram,
			const vaddr_t offs,
			const uint32_t value,
			const memory_access_width_t aw)
{
  if(aw == WORD) {
    write_ram(ram, offs, &value, 1, aw);
  } else if(aw == HALFWORD) {
    uint16_t val = (uint16_t)(value&0xffff);
    write_ram(ram, offs, &val, 1, aw);
  } else {
    uint8_t val = (uint8_t)(value&0xff);
    write_ram(ram, offs, &val, 1, aw);
  }
}
