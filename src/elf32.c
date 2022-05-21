
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "elf32.h"

Elf32 *elf_load (const uint8_t *elf_start, mmu_t *mmu)
{
  Elf32 *elf = malloc(sizeof(Elf32));
  if(!elf) {
    return NULL;
  }
  memset(elf, 0, sizeof(Elf32));

  Elf32_Ehdr *hdr = (Elf32_Ehdr *) elf_start;
  assert(hdr->e_ident[EI_CLASS] == 1);
  assert(hdr->e_ident[EI_DATA] == 1);
  Elf32_Phdr *phdr = (Elf32_Phdr *)(elf_start + hdr->e_phoff);

  // Load program into memory
  for(int i=0; i < hdr->e_phnum; i++) {
    if(phdr[i].p_type == 1) { // LOAD
      if(!mmu_add_memory(mmu, phdr[i].p_vaddr, phdr[i].p_memsz, MPERM_WRITE|MPERM_RAW)) {
	fprintf(stderr, "elf: could not add memory 0x%08x size 0x%08x\n", phdr[i].p_vaddr, phdr[i].p_memsz);
	free(elf);
	return NULL;
      }

      mmu_write_from(mmu,
		     (void *)(elf_start + phdr[i].p_offset),
		     phdr[i].p_vaddr,
		     phdr[i].p_filesz);

      const Elf32_Word f = phdr[i].p_flags;
      const mperm_t perm = ((f&4) != 0 ? MPERM_READ : 0) | 
	((f&2) != 0 ? MPERM_WRITE : 0) |
	((f&1) != 0 ? MPERM_EXEC : 0) |
	((f&4) != 0 && (f&2) == 0 ? MPERM_RAW : 0);
      mmu_setperm(mmu, phdr[i].p_vaddr, phdr[i].p_filesz, perm);
    }
  }
  elf->entry = hdr->e_entry;
  return elf;
}
