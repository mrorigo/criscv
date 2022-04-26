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
#include <stdlib.h>
#include <assert.h>
#include "bus.h"

mmio_device_t *bus_find_device(const bus_t *bus, const size_t offs)
{
  mmio_device_t *dev = bus->mmio_devices;
  while(dev != NULL) {
    fprintf(stderr, "bus_find_device 0x%08zx  0x%08x => 0x%08x\n", offs, dev->base_address, (dev->base_address + (dev->size)));
    if((dev->base_address <= offs) &&
       (dev->base_address + (dev->size)) > offs) {
      return dev;
    }
    dev = dev->next;
  }
  fprintf(stderr, "FATAL: bus_find_device %08zx\n", offs);
  assert(dev != NULL);
  return NULL;
}

void bus_init(const bus_t *bus)
{
  mmio_device_t *dev = bus->mmio_devices;
  while(dev != NULL) {
    dev->init(dev);
    dev = dev->next;
  }
}

uint32_t bus_read(const bus_t *bus, const size_t offs,  const memory_access_width_t aw)
{
  const mmio_device_t *dev = bus_find_device(bus, offs);
  return dev->read(dev, offs, aw);
}

void bus_write(const bus_t *bus, const size_t offs, const uint32_t value, const memory_access_width_t aw)
{
  mmio_device_t *dev = bus_find_device(bus, offs);
  dev->write(dev, offs, value, aw);
}
