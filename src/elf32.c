
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
  Elf32_Shdr *shdr = (Elf32_Shdr *)(elf_start + hdr->e_shoff);
  const Elf32_Shdr *sh_strtab = &shdr[hdr->e_shstrndx];
  const char *const sh_strtab_p = elf_start + sh_strtab->sh_offset;

  const size_t page_aligned_size = get_total_page_aligned_size(elf_start);
  
  elf->load = mmu_allocate(mmu, page_aligned_size, MPERM_EXEC|MPERM_WRITE);
  assert(elf->load);
  fprintf(stderr, "_elf: size: 0x%08zx alloced at 0x%08x with perm 0x%02x\n",
	  page_aligned_size, elf->load, MPERM_EXEC|MPERM_WRITE);

  elf->base = elf->load;
  //  fprintf(stderr, "_elf: hdr->e_shnum %zu\n", hdr->e_shnum);

#define MAP_ADDR(x) (x + (elf->base - elf->load))
  //#define MAP_ADDR(x) (x + (elf->base - elf->load))

  elf->entry = MAP_ADDR(hdr->e_entry);
  fprintf(stderr, "_elf: entry 0x%08x (0x%08x)\n", elf->entry, hdr->e_entry);
  
  for(size_t i=0; i < hdr->e_shnum; ++i) {

    if (shdr[i].sh_type == SHT_INIT_ARRAY) { // 14
      assert(elf->base != 0);
      elf->init_count = shdr[i].sh_size / sizeof(uint32_t);
      elf->init_array = MAP_ADDR(shdr[i].sh_addr);
      fprintf(stderr, "_elf: init_array: 0x%08x\n", elf->init_array);

      mmu_write_from(mmu,
		     (void *)(elf_start + shdr[i].sh_offset),
		     elf->init_array,
		     shdr[i].sh_size);

      mmu_setperm(mmu, elf->init_array, shdr[i].sh_size, MPERM_READ|MPERM_EXEC);
    }
    else if (shdr[i].sh_type == SHT_FINI_ARRAY) { // 15
      assert(elf->base != 0);
      elf->fini_count = shdr[i].sh_size / sizeof(uint32_t);
      elf->fini_array = MAP_ADDR(shdr[i].sh_addr);
      fprintf(stderr, "_elf: fini_array: 0x%08x\n", elf->fini_array);

      mmu_write_from(mmu,
		     (void *)(elf_start + shdr[i].sh_offset),
		     elf->fini_array,
		     shdr[i].sh_size);

      mmu_setperm(mmu, elf->fini_array, shdr[i].sh_size, MPERM_READ|MPERM_EXEC);

    }
    else if(shdr[i].sh_size > 0 &&
	    shdr[i].sh_type == SHT_PROGBITS &&
	    shdr[i].sh_addr != 0) {

      const Elf32_Word addr_align = shdr[i].sh_addralign;
      const Elf32_Word ent_size = shdr[i].sh_entsize;
      const vaddr_t mem_addr = MAP_ADDR( shdr[i].sh_addr );
      
      fprintf(stderr, "_elf: load section %s of size 0x%08x from offs 0x%08x to addr (0x%08x)0x%08x->0x%08x  addr_align=0x%08x ent_size=0x%08x\n",
	      sh_strtab_p + shdr[i].sh_name,
	      shdr[i].sh_size,
	      shdr[i].sh_offset,
	      shdr[i].sh_addr,
	      mem_addr,
	      mem_addr+shdr[i].sh_size,
	      addr_align,
	      ent_size);
      if(strcmp(".sdata", sh_strtab_p + shdr[i].sh_name) == 0) {
	elf->global_pointer = mem_addr + 0x800; // from riscv/link.ld
	fprintf(stderr, "_elf: global_pointer set to 0x%08x\n", elf->global_pointer);
      }

      mmu_write_from(mmu,
		     (void *)(elf_start + shdr[i].sh_offset),
		     mem_addr,
		     shdr[i].sh_size);

      const Elf32_Word sh_flags = shdr[i].sh_flags;
      mperm_t perm = MPERM_READ;
      if((SHF_EXECINSTR & sh_flags) == SHF_EXECINSTR) {
	perm |= MPERM_EXEC;
      } 
      if((SHF_WRITE & sh_flags) == SHF_WRITE) {
	perm |= MPERM_WRITE;
      } 
	
      mmu_setperm(mmu,
      		  MAP_ADDR(shdr[i].sh_addr),
      		  shdr[i].sh_size, //(((shdr[i].sh_size)>>2)+1) << 2,
      		  perm);
    }
    else if(shdr[i].sh_type == SHT_NOBITS) { // bss
      fprintf(stderr, "_elf: init BSS section %s @ 0x%08x, size %d\n",
	      sh_strtab_p + shdr[i].sh_name, MAP_ADDR(shdr[i].sh_addr), shdr[i].sh_size);
      mmu_setperm(mmu,
      		  MAP_ADDR(shdr[i].sh_addr),
      		  shdr[i].sh_size,
      		  MPERM_READ|MPERM_WRITE);
    }
    else {

      fprintf(stderr, "_elf: skipped section %s, type %d\n",
	      sh_strtab_p + shdr[i].sh_name, shdr[i].sh_type);
    }
  }
#undef MAP_ADDR

  return elf;
}
