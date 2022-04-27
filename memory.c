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
#include "memory.h"
#include "mmio.h"
#include <assert.h>

void init_ram_rom(mmio_device_t *ram)
{
  ram->user = malloc(ram->size * sizeof(uint32_t));
}

uint32_t read_ram_rom(const mmio_device_t *ram, const uint32_t offs, const memory_access_width_t aw)
{
  //  fprintf(stderr, "memory::read_ram_rom: %08x (%u)\n", offs, aw);
  if(aw == WORD) {
    assert((offs & 3) == 0);
    return ((uint32_t*)ram->user)[(offs>>2) & (ram->size-1)];
  } else if(aw == HALFWORD) {
    assert((offs & 1) == 0);
    return (uint32_t)((uint16_t*)ram->user)[(offs>>1) & ((ram->size<<1)-1)];
  } else if(aw == BYTE) {
    return (uint32_t)((uint8_t*)ram->user)[(offs) & ((ram->size<<2)-1)];
  }
  assert(0==1);
}

void write_ram_rom(mmio_device_t *ram, const uint32_t offs, const uint32_t value, const memory_access_width_t aw)
{
  // TODO: Do not allow write to ROM (unless loading)
  //  fprintf(stderr, "memory::write_ram_rom: %08x = %08x (%u)\n", offs, value, aw);
  if(aw == WORD) {
    assert((offs & 3) == 0);
    ((uint32_t *)ram->user)[(offs>>2) & (ram->size-1)] = value;
  } else if(aw == HALFWORD) {
    assert((offs & 1) == 0);
    ((uint16_t *)ram->user)[(offs>>1) & ((ram->size<<1)-1)] = (uint16_t)(value & 0xffff);
  } else if(aw == BYTE) {
    ((uint8_t *)ram->user)[(offs) & ((ram->size<<2)-1)] = (uint8_t)(value & 0xff);
  }
}
