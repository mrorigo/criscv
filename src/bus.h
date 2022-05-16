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
#ifndef __BUS_H__
#define __BUS_H__

#include <sys/types.h>
#include <pthread.h>
#include "mmio.h"

typedef enum _bus_status_t {
  OK = 0,
  READ_MISALIGNED   = 1,
  WRITE_MISALIGNED  = 2,
  ADDRESS_NOT_FOUND = 4,
} bus_status_t;

typedef struct _bus_t {
  mmio_device_t *mmio_devices;
  pthread_mutex_t mutex;
  bus_status_t status;
} bus_t;

void     bus_init(bus_t *);
uint32_t bus_read_single(bus_t *, const size_t,  const memory_access_width_t);
void     bus_write_single(bus_t *, const size_t, const uint32_t, const memory_access_width_t);

void bus_read_multiple(bus_t *, const size_t, void *, size_t, const memory_access_width_t);
void bus_write_multiple(bus_t *bus, const size_t offs, void *src, size_t count,
                        const memory_access_width_t aw);

void     bus_begin_write(bus_t *);
void     bus_end_write(bus_t *);

void     bus_begin_read(bus_t *);
void     bus_end_read(bus_t *);

#endif
