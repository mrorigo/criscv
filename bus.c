#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bus.h"

mmio_device_t *bus_find_device(const bus_t *bus, const size_t offs)
{
  mmio_device_t *dev = bus->mmio_devices;
  while(dev != NULL) {
    if((dev->base_address <= offs) &&
       (dev->base_address + (dev->size >> 2)) > offs) {
      return dev;
    }
    dev = dev->next;
  }
  fprintf(stderr, "FATAL: bus_find_device %08zx\n", offs);
  assert(dev != NULL);
  return NULL;
}

void bus_init(bus_t *bus)
{
  mmio_device_t *dev = bus->mmio_devices;
  while(dev != NULL) {
    dev->init(dev);
    dev = dev->next;
  }
}

uint32_t bus_read(const bus_t *bus, const size_t offs)
{
  const mmio_device_t *dev = bus_find_device(bus, offs);
  return dev->read(dev, offs);
}

void bus_write(const bus_t *bus, const size_t offs, const uint32_t value)
{
  mmio_device_t *dev = bus_find_device(bus, offs);
  dev->write(dev, offs, value);
}
