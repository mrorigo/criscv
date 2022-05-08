#ifndef __MMU_H__
#define __MMU_H__

#include <stdbool.h>
#include <sys/types.h>

typedef enum _mperm_t {
  MPERM_READ	= 1,
  MPERM_WRITE	= 1<<1,
  MPERM_EXEC	= 1<<2,
  MPERM_RAW	= 1<<3
} mperm_t;

typedef uint32_t vaddr_t;
    
typedef struct _mmu_t {
  size_t  size;
  vaddr_t base;
  size_t  curr_vaddr;
  void    *data;
  mperm_t *perm;
  bool    *dirty;
} mmu_t;

#define DIRTY_PAGE_SIZE 64

bool check_has_access(const mmu_t	*mmu,
		      const vaddr_t	 vaddr,
		      const size_t	 size_in_bytes,
		      const mperm_t	 perm);

mmu_t	*mmu_init(const vaddr_t base, const size_t size);
vaddr_t	 mmu_allocate(mmu_t *mmu, const size_t size, mperm_t perm);
vaddr_t	 mmu_allocate_raw(mmu_t *mmu, const size_t size);
int	 mmu_write_from(mmu_t *mmu, void *src, vaddr_t vaddr, size_t size_in_bytes);
int	 mmu_read_into(mmu_t *mmu, void *dst, vaddr_t vaddr, size_t size_in_bytes);

#endif
