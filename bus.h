#ifndef __BUS_H__
#define __BUS_H__

#include <sys/types.h>
#include "mmio.h"

typedef struct _bus_t {
  mmio_device_t *mmio_devices;
} bus_t;

void bus_init(bus_t *bus);
uint32_t bus_read(const bus_t *bus, const size_t offs);
void bus_write(const bus_t *bus, const size_t offs, const uint32_t value);

#endif
