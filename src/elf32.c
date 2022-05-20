
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "elf32.h"

static size_t get_total_page_aligned_size(const uint8_t *elf_start)
{
  Elf32_Ehdr *hdr = (Elf32_Ehdr *) elf_start;
  assert(hdr->e_ident[EI_CLASS] == 1);
  assert(hdr->e_ident[EI_DATA] == 1);
  Elf32_Shdr *shdr = (Elf32_Shdr *)(elf_start + hdr->e_shoff);

  vaddr_t min_vaddr = 0x7fffffff;
  vaddr_t max_vaddr = 0x0;

  for(size_t i=0; i < hdr->e_shnum; ++i) {
    if(shdr[i].sh_addr != 0) {
      min_vaddr = shdr[i].sh_addr < min_vaddr ? shdr[i].sh_addr : min_vaddr;
      max_vaddr = shdr[i].sh_addr + shdr[i].sh_size > max_vaddr ?
	shdr[i].sh_addr + shdr[i].sh_size : max_vaddr;
    }
  }
  fprintf(stderr, "min_vaddr: 0x%08x  max_vaddr: 0x%08x\n", min_vaddr, max_vaddr);
  const size_t page_aligned_virt_size = (((max_vaddr - min_vaddr)>>12) + 1)<<12;

  return page_aligned_virt_size;
}

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
  //  Elf32_Shdr *shdr = (Elf32_Shdr *)(elf_start + hdr->e_shoff);
  Elf32_Phdr *phdr = (Elf32_Phdr *)(elf_start + hdr->e_phoff);
  //  const Elf32_Shdr *sh_strtab = &shdr[hdr->e_shstrndx];
  //  const char *const sh_strtab_p = elf_start + sh_strtab->sh_offset;

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
      Elf32_Word f = phdr[i].p_flags;
      mperm_t perm = (f&4) != 0 ? MPERM_READ : 0;
      perm |= ((f&2) != 0 ? MPERM_WRITE : 0);
      perm |= ((f&1) != 0 ? MPERM_EXEC : 0);
      mmu_setperm(mmu, phdr[i].p_vaddr, phdr[i].p_filesz, perm);
    }
  }
  elf->entry = hdr->e_entry;
  return elf;
}
