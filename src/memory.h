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
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "mmu.h"
#include "mmio.h"

void init_ram(mmio_device_t *ram);
uint32_t read_ram(const mmio_device_t *ram, const uint32_t offs, const memory_access_width_t aw);
void write_ram(mmio_device_t *ram, const uint32_t offs, const uint32_t value, const memory_access_width_t aw);

void dump_ram_rom(const mmio_device_t *ram,
		  const uint32_t offs,
		  const uint32_t count,
		  const memory_access_width_t aw);

#endif
