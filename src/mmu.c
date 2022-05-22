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

#include "mmu.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <assert.h>

void mmu_setperm(mmu_t *mmu, const vaddr_t vaddr, const size_t size, const mperm_t perm)
{
  fprintf(stderr, "mmu::setperm vaddr=0x%08x->0x%08lx, perm=0x%02x, size/aligned=0x%08zx/0x%08lx\n", vaddr, vaddr+size, perm, size, size);
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
  memset(mmu->data, 0x0, size);

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

bool mmu_add_memory(mmu_t *mmu, const vaddr_t addr, const size_t size, const mperm_t perm)
{
  if(addr < mmu->base) {
    assert(0 && "can not extend memory segment");
    return false;
  }
  if(addr + size > mmu->base + mmu->size) {
    // Extend the current memory segment
    const size_t new_size = (addr+size) - mmu->base;
    void * new_data = realloc(mmu->data, new_size);
    if(new_data == NULL) {
      assert(new_data);
      return false;
    }
    mmu->data = new_data;
    mmu->size = new_size;
  } else {
    // extend the curr_vaddr beyond addr+size
    mmu->curr_vaddr = addr + size;
  }
  mmu_setperm(mmu, addr, size, perm);
  return true;
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
  mmu->curr_vaddr = curr_vaddr + aligned_size;
  assert(mmu->curr_vaddr > aligned_size); // overflow
  
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

    if((p & MPERM_READ) == perm ||
       (p & MPERM_WRITE) == perm ||
       (p & MPERM_EXEC) == perm) {
      continue;
    }
    else if((p & MPERM_RAW) == MPERM_RAW && perm == MPERM_WRITE) {
      continue;
    }
    else {
      fprintf(stderr, "mmu::check_access: Access denied to memory at 0x%08x with permissions 0x%02x. Requested permission: 0x%02x\n", (unsigned int)i + mmu->base, p, perm);
      return false;
    }
  }
  return true;
}

size_t mmu_write_from(mmu_t *mmu, const void *src, const vaddr_t vaddr, const size_t size_in_bytes)
{
  if(vaddr < mmu->base || vaddr + size_in_bytes > mmu->base + mmu->size) {
    mmu->state = READ_PAGE_FAULT;
    return 0;
  }
  assert(vaddr >= mmu->base);
  assert(vaddr <= mmu->base + mmu->size );
  if(!mmu_check_access(mmu, vaddr, size_in_bytes, MPERM_WRITE)) {
    mmu->state = ACCESS_DENIED;
    return 0;
  }
  mmu->state = MMU_OK;
  memcpy((uint8_t*)mmu->data + (vaddr - mmu->base), src, size_in_bytes);

  // Update perms
  for(size_t i = vaddr-mmu->base; i < vaddr-mmu->base+size_in_bytes; i++) {
    if((mmu->perm[i] & MPERM_RAW) == MPERM_RAW) {
      mmu->perm[i] = MPERM_READ|MPERM_WRITE|(mmu->perm[i]&MPERM_EXEC);
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

size_t mmu_read_into(mmu_t *mmu,
		     void *dst,
		     vaddr_t vaddr,
		     size_t size_in_bytes)
{
  if(vaddr < mmu->base || vaddr + size_in_bytes > mmu->base + mmu->size) {
    mmu->state = WRITE_PAGE_FAULT;
    return 0;
  }

  if(mmu_check_access(mmu, vaddr, size_in_bytes, MPERM_READ)) {
    memcpy(dst, (uint8_t*)mmu->data + (vaddr - mmu->base), size_in_bytes);
    mmu->state = MMU_OK;
  } else {
    fprintf(stderr, "mmu::read_into from 0x%08x, size %zu, access denied!\n", vaddr, size_in_bytes);
    mmu->state = ACCESS_DENIED;
    return 0;
  }
  return size_in_bytes;
}
