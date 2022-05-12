#ifndef __MMU_H__
#define __MMU_H__

#include <stdbool.h>
#include <sys/types.h>

typedef enum _mperm_t {
  MPERM_READ	= 1,
  MPERM_WRITE	= 2,
  MPERM_EXEC	= 4,
  MPERM_RAW	= 8
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

bool     mmu_check_access(const mmu_t *,
			  const vaddr_t,
			  const size_t,
			  const mperm_t);
mmu_t	*mmu_init(const vaddr_t, const size_t);
vaddr_t	 mmu_allocate(mmu_t *, const size_t, mperm_t);
vaddr_t	 mmu_allocate_raw(mmu_t *, const size_t);
int	 mmu_write_from(mmu_t *, void *, vaddr_t, size_t);
int	 mmu_read_into(mmu_t *, void *, vaddr_t, size_t);
void	 mmu_setperm(mmu_t *, const vaddr_t, const size_t, const mperm_t);

#endif
