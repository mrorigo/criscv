#ifndef __MMIO_H__
#define __MMIO_H__

#include <sys/types.h>

typedef enum __attribute((packed)) _memory_access_width_t {
  BYTE, HALFWORD, WORD
} memory_access_width_t;

typedef struct _mmio_device_t {
  struct _mmio_device_t *next;

  uint32_t base_address; 
  uint32_t size; // in 32bit words
  void    *user;

  void     (*init )(struct _mmio_device_t *device);
  uint32_t (*read )(const struct _mmio_device_t *device, const uint32_t offs, const memory_access_width_t aw);
  void     (*write)(struct _mmio_device_t *device, const uint32_t offs, const uint32_t value, const memory_access_width_t aw);

} mmio_device_t;

#endif
