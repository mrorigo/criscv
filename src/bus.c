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
#include <pthread.h>
#include "bus.h"

void bus_init(bus_t *bus)
{
  mmio_device_t *dev = bus->mmio_devices;
  while(dev != NULL) {
    dev->init(dev);
    dev = dev->next;
  }

  pthread_mutex_init(&bus->mutex, NULL);
}

static void bus_lock_read(bus_t *bus)
{
  pthread_mutex_lock(&bus->mutex);
}

static void bus_lock_write(bus_t *bus)
{
  pthread_mutex_lock(&bus->mutex);
}

static void bus_unlock_read(bus_t *bus)
{
  pthread_mutex_unlock(&bus->mutex);
}

static void bus_unlock_write(bus_t *bus)
{
  pthread_mutex_unlock(&bus->mutex);
}

static mmio_device_t *bus_find_device(bus_t *bus, const size_t offs)
{
  mmio_device_t *dev = bus->mmio_devices;
  bus->status = BUS_OK;
  while(dev != NULL) {
    if((dev->base_address <= offs) &&
       (dev->base_address + (dev->size)) > offs) {
      return dev;
    }
    dev = dev->next;
  }
  bus->status = BUS_ADDRESS_NOT_FOUND;
  return NULL;
}

void bus_begin_write(bus_t *bus)
{
  bus_lock_write(bus);
}

void bus_end_write(bus_t *bus)
{
  bus_unlock_write(bus);
}

void bus_begin_read(bus_t *bus)
{
  bus_lock_read(bus);
}

void bus_end_read(bus_t *bus)
{
  bus_unlock_read(bus);
}

size_t bus_read_string(bus_t *bus, const size_t offs, char *dst)
{
  char c = 0;
  size_t i = 0;
  *dst = 0;
  do {
    c = bus_read_single(bus, offs+i, BYTE);
    dst[i++] = c;
  } while(c != 0 && bus->status == BUS_OK);

  return i;
}

uint32_t bus_read_single(bus_t *bus, const size_t offs, const memory_access_width_t aw)
{
  bus_begin_read(bus);
  mmio_device_t *dev = bus_find_device(bus, offs);
  if(bus->status != BUS_OK) {
    bus_end_read(bus);
    return 0x0badc0de;
  }
  uint32_t r = dev->read_single(dev, offs, aw);
  bus_end_read(bus);
  return r;
}

size_t bus_read_multiple(bus_t *bus, const size_t offs, void *dst, size_t count, const memory_access_width_t aw)
{
  bus_begin_read(bus);
  mmio_device_t *dev = bus_find_device(bus, offs);
  if(bus->status != BUS_OK) {
    #ifdef BUS_TRACE
    fprintf(stderr, "bus:read_multiple:bus_find_device failed\n");
    #endif
    bus_end_read(bus);
    return 0;
  }
  bus->status = BUS_OK;
  if(dev->read(dev, offs, dst, count, aw) != count) {
    #ifdef BUS_TRACE
    fprintf(stderr, "bus:read_multiple:dev_read failed: %d\n", dev->state);
    #endif
    bus->status = BUS_DEVICE_FAILURE;
    bus_end_read(bus);
    return 0;
  }
  bus_end_read(bus);
  return count;
}

void bus_write_single(bus_t *bus, const size_t offs, const uint32_t value, const memory_access_width_t aw)
{
  bus_begin_write(bus);
  mmio_device_t *dev = bus_find_device(bus, offs);
  if(bus->status != BUS_OK) {
    bus_end_write(bus);
    return;
  }
  if((dev->perm & WRITE) != WRITE) {
    // TODO: Raise trap
    assert((dev->perm & WRITE) == WRITE);
    return;
  }
  dev->write_single(dev, offs, value, aw);
  bus_end_write(bus);
}

size_t bus_write_multiple(bus_t *bus, const size_t offs, void *src, size_t count, const memory_access_width_t aw)
{
  bus_begin_write(bus);
  mmio_device_t *dev = bus_find_device(bus, offs);
  if(bus->status != BUS_OK || dev == NULL) {
    bus_end_write(bus);
    return 0;
  }
  if((dev->perm & WRITE) != WRITE) {
    bus->status = BUS_ACCESS_DENIED;
    // TODO: Raise trap
    assert((dev->perm & WRITE) == WRITE);
    return 0;
  }

  assert(dev->state == READY);
  if(dev->write(dev, offs, src, count, aw) != count) {
    //bus->status = dev->state;
    bus->status = BUS_DEVICE_FAILURE;
    return 0;
  }
  bus->status = BUS_OK;
  bus_end_write(bus);
  return count;
}
