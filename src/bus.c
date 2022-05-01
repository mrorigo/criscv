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

  bus->readers = 0;
  bus->writers = 0;
  bus->writers_waiting = 0;

  pthread_cond_init(&bus->read_cond, NULL);
  pthread_cond_init(&bus->write_cond, NULL);
  pthread_mutex_init(&bus->mutex, NULL);
}

static void bus_lock_read(bus_t *bus)
{
  pthread_mutex_lock(&bus->mutex);
  while(bus->writers>0) {
    //    fprintf(stderr, "bus_lock_read: bus->writers = %d\n", bus->writers);
    pthread_cond_wait(&bus->read_cond, &bus->mutex);
  }
  bus->readers++;
  pthread_mutex_unlock(&bus->mutex);
}

static void bus_lock_write(bus_t *bus)
{
  pthread_mutex_lock(&bus->mutex);
  bus->writers_waiting++;
  while(bus->readers > 0 || bus->writers > 0) {
    //    fprintf(stderr, "bus_lock_write: readers: %d  writers; %d (%d waiting)\n", bus->readers, bus->writers, bus->writers_waiting);
    pthread_cond_wait(&bus->write_cond, &bus->mutex);
  }
  bus->writers_waiting--;
  bus->writers++;
  pthread_mutex_unlock(&bus->mutex);
}

static void bus_unlock_read(bus_t *bus)
{
  pthread_mutex_lock(&bus->mutex);
  assert(bus->readers > 0);
  bus->readers--;
  if(bus->readers == 0 && bus->writers_waiting > 0) {
    pthread_cond_signal(&bus->write_cond);
  }
  pthread_mutex_unlock(&bus->mutex);
}

static void bus_unlock_write(bus_t *bus)
{
  pthread_mutex_lock(&bus->mutex);
  assert(bus->writers > 0);
  bus->writers--;
  if(bus->writers_waiting > 0) {
    pthread_cond_signal(&bus->write_cond);
  }
  
  pthread_cond_broadcast(&bus->read_cond);
  pthread_mutex_unlock(&bus->mutex);
}

static mmio_device_t *bus_find_device(bus_t *bus, const size_t offs)
{
  mmio_device_t *dev = bus->mmio_devices;
  while(dev != NULL) {
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

uint32_t bus_read_single(bus_t *bus, const size_t offs, const memory_access_width_t aw)
{
  bus_begin_read(bus);
  const mmio_device_t *dev = bus_find_device(bus, offs);
  uint32_t r = dev->read(dev, offs, aw);
  bus_end_read(bus);
  return r;
}

void bus_read_multiple(bus_t *bus, const size_t offs, void *dst, size_t count, const memory_access_width_t aw)
{
  bus_begin_read(bus);
  const mmio_device_t *dev = bus_find_device(bus, offs);
  size_t o = offs;

  // fast path for instruction prefetch
  if(aw == WORD && count == 4) {
    uint32_t *ptr = (uint32_t *)dst;
    ptr[0] = dev->read(dev, o, aw);
    ptr[1] = dev->read(dev, o+4, aw);
    ptr[2] = dev->read(dev, o+8, aw);
    ptr[3] = dev->read(dev, o+12, aw);
    bus_end_read(bus);
    return;
 }

  if(aw == WORD) {
    uint32_t *ptr = (uint32_t *)dst;
    for(size_t i=0; i < count; i++) {
      ptr[i] = dev->read(dev, o, aw);
      o += 4;
    }
  } else if(aw == HALFWORD) {
    uint16_t *ptr = (uint16_t *)dst;
    for(size_t i=0; i < count; i++) {
      ptr[i] = dev->read(dev, o, aw);
      o += 2;
    }
  } else  if(aw == BYTE) {
    uint8_t *ptr = (uint8_t *)dst;
    for(size_t i=0; i < count; i++) {
      ptr[i] = dev->read(dev, o, aw);
      o += 1;
    }
  }
  bus_end_read(bus);
}

void bus_write_single(bus_t *bus, const size_t offs, const uint32_t value, const memory_access_width_t aw)
{
  bus_begin_write(bus);
  mmio_device_t *dev = bus_find_device(bus, offs);
  if((dev->perm & WRITE) != WRITE) {
    // TODO: Raise trap
    assert((dev->perm & WRITE) == WRITE);
    return;
  }
  dev->write(dev, offs, value, aw);
  bus_end_write(bus);
}