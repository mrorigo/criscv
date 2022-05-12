
#include "mmu.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <assert.h>

void mmu_setperm(mmu_t *mmu, const vaddr_t vaddr, const size_t size, const mperm_t perm)
{
  fprintf(stderr, "mmu::setperm vaddr=0x%08x, size=%zu, perm=0x%02x\n", vaddr, size, perm);
  assert(vaddr >= mmu->base);
  assert(vaddr+size <= mmu->base+mmu->size);
  for(size_t i=vaddr-mmu->base; i < vaddr-mmu->base+size; i++) {
    mmu->perm[i] = perm;
  }
}

// Initialize a new MMU, initializing
mmu_t *mmu_init(const vaddr_t base, const size_t size)
{
  mmu_t *mmu = malloc(sizeof(mmu_t));

  if(!mmu) {
    return NULL;
  }

  mmu->base = base;
  mmu->size = size;
  mmu->curr_vaddr = base;

  mmu->data = malloc(size);
  if(!mmu->data) {
    free(mmu);
    return NULL;
  }
  memset(mmu->data, 0xfa, size);

  const size_t dirty_size = (size / DIRTY_PAGE_SIZE) + 1;
  mmu->dirty = malloc(dirty_size);
  if(!mmu->dirty) {
    free(mmu->data);
    free(mmu);
    return NULL;
  }
  memset(mmu->dirty, 0, dirty_size); 

  mmu->perm = malloc(sizeof(mperm_t)*size);
  if(!mmu->perm) {
    free(mmu->dirty);
    free(mmu->data);
    free(mmu);
    return NULL;
  }
  mmu_setperm(mmu, mmu->base, mmu->size, MPERM_WRITE|MPERM_RAW);

  return mmu;
}

// Allocate a block of memory with specific access permissions
vaddr_t mmu_allocate(mmu_t *mmu, const size_t size, mperm_t perm)
{
  const size_t aligned_size = (size + 0x0f) & (0xfffffff0);
  const uint32_t curr_vaddr = mmu->curr_vaddr;

  if(curr_vaddr+aligned_size >= mmu->base + mmu->size) {
    fprintf(stderr, "mmu::allocate_rw: Out of memory at 0x%08x, size=%zu\n", curr_vaddr, mmu->size);
    return 0;
  }
  mmu->curr_vaddr = curr_vaddr + aligned_size; // TODO: overflow
  mmu_setperm(mmu, curr_vaddr, size, perm);

  fprintf(stderr, "mmu::allocate_rw: size=%zu, aligned_size=%zu  addr=0x%08x\n", size, aligned_size, curr_vaddr);

  return curr_vaddr;
}

// Allocate a block of ReadAfterWrite memory
vaddr_t mmu_allocate_raw(mmu_t *mmu, const size_t size)
{
  return mmu_allocate(mmu, size, MPERM_WRITE|MPERM_RAW);
}

bool mmu_check_access(const mmu_t *mmu,
		      const vaddr_t vaddr,
		      const size_t size_in_bytes,
		      const mperm_t perm)
{
  for(size_t i=vaddr-mmu->base; i < vaddr - mmu->base + size_in_bytes; i++) {
    const mperm_t p = mmu->perm[i];
    if((p & perm) != perm) {
      if((p & MPERM_RAW) == MPERM_RAW) {
	fprintf(stderr, "mmu::check_access: Read of uninitialized RAW memory at 0x%08x\n", (unsigned int)i + mmu->base);
	exit(99);
      } else {
	fprintf(stderr, "mmu::check_access: Access denied to memory at 0x%08x. perm=%02x, requested perm=%02x\n", (vaddr_t)(vaddr+i), MPERM_RAW, perm);
	assert(false);
      }
      return false;
    }
  }
  return true;
}

int mmu_write_from(mmu_t *mmu, void *src, const vaddr_t vaddr, const size_t size_in_bytes)
{
  assert(vaddr >= mmu->base);
  assert(vaddr <= mmu->base + mmu->size );
  if(!mmu_check_access(mmu, vaddr, size_in_bytes, MPERM_WRITE)) {
    return 0;
  }
  
  memcpy((uint8_t*)mmu->data + (vaddr - mmu->base), src, size_in_bytes);

  // Update perms
  for(size_t i = vaddr-mmu->base; i < vaddr-mmu->base+size_in_bytes; i++) {
    if((mmu->perm[i] & MPERM_RAW) == MPERM_RAW) {
      mmu->perm[i] |= MPERM_READ;
    }
  }

  // Mark dirty
  const size_t dbi = (vaddr - mmu->base) / DIRTY_PAGE_SIZE;
  const size_t dbe = (vaddr - mmu->base + size_in_bytes) / DIRTY_PAGE_SIZE;
  for(size_t i=dbi; i < dbe; i++) {
    mmu->dirty[i] = true;
  }
  return size_in_bytes;
}

int mmu_read_into(mmu_t *mmu,
		  void *dst,
		  vaddr_t vaddr,
		  size_t size_in_bytes)
{
  assert(vaddr < mmu->base + mmu->size);
  assert(mmu->base <= vaddr);
  if(mmu_check_access(mmu, vaddr, size_in_bytes, MPERM_READ)) {
    memcpy(dst, (uint8_t*)mmu->data + (vaddr - mmu->base), size_in_bytes);
  } else {
    fprintf(stderr, "mmu::read_into from 0x%08x, size %zu, access denied!\n", vaddr, size_in_bytes);
    assert(false);
  }
  return size_in_bytes;
}
