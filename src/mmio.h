#ifndef __MMIO_H__
#define __MMIO_H__

#include <sys/types.h>

typedef enum __attribute((packed)) _memory_access_width_t {
  BYTE, HALFWORD, WORD
} memory_access_width_t;

typedef enum _mmio_perm_t {
  READ	= 1<<0,
  WRITE	= 1<<1
} mmio_perm_t;

typedef enum _device_state_t {
  READY,
  BUSY,
  ERROR
} device_state_t;

typedef struct _mmio_device_t {
  struct _mmio_device_t *next;

  uint32_t base_address; 
  uint32_t size; // in 32bit words
  void    *user;
  mmio_perm_t perm;
  device_state_t state;
      
  void     (*init )(struct _mmio_device_t *device);
  uint32_t (*read_single)(struct _mmio_device_t *device,
			   const uint32_t offs,
			   const memory_access_width_t aw);

  size_t   (*read)(struct _mmio_device_t *device,
		   const uint32_t offs,
		   void *buf,
		   const size_t size,
		   const memory_access_width_t aw);
  

  void   (*write_single)(struct _mmio_device_t *device,
			   const uint32_t offs,
			   const uint32_t value,
			   const memory_access_width_t aw);

  size_t     (*write)(struct _mmio_device_t *device,
		    const uint32_t offs,
		    const void *buf,
		    size_t count,
		    const memory_access_width_t aw);

} mmio_device_t;

#endif
