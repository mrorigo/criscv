
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "elf32.h"

/* void *resolve(const char* sym) */
/* { */
/*   static void *handle = NULL; */
/*   if (handle == NULL) { */
/*     handle = dlopen("libc.so", RTLD_NOW); */
/*   } */
/*   return dlsym(handle, sym); */
/* } */

// TODO: relocation
/* void relocate(Elf32_Shdr* shdr, const Elf32_Sym* syms, const char* strings, const char* src, char* dst) */
/* { */
/*   Elf32_Rel* rel = (Elf32_Rel*)(src + shdr->sh_offset); */
/*   int j; */
/*   for(j = 0; j < shdr->sh_size / sizeof(Elf32_Rel); j += 1) { */
/*     const char* sym = strings + syms[ELF32_R_SYM(rel[j].r_info)].st_name; */
/*     switch(ELF32_R_TYPE(rel[j].r_info)) { */
/*     case R_386_JMP_SLOT: */
/*     case R_386_GLOB_DAT: */
/*       *(Elf32_Word*)(dst + rel[j].r_offset) = (Elf32_Word)resolve(sym); */
/*       break; */
/*     } */
/*   } */
/* } */

void* find_sym(const char* name,
	       Elf32_Shdr* shdr,
	       char* strings,
	       const unsigned char* src)
{
  Elf32_Sym* syms = (Elf32_Sym*)(src + shdr->sh_offset);
  for(size_t i = 0; i < shdr->sh_size / sizeof(Elf32_Sym); i += 1) {
    //    fprintf(stderr, "_sym: %s\n", strings + syms[i].st_name);
    if (strcmp(name, strings + syms[i].st_name) == 0) {
      return (void *)((uint64_t)syms[i].st_value);
    }
  }
  return NULL;
}

Elf32 *elf_load (uint8_t *elf_start, mmu_t *mmu)
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

  
  vaddr_t min_vaddr = 0x7fffffff;
  vaddr_t max_vaddr = 0x0;

  for(size_t i=0; i < hdr->e_shnum; ++i) {
    if(shdr[i].sh_type == SHT_PROGBITS && shdr[i].sh_addr != 0) {
      min_vaddr = shdr[i].sh_addr < min_vaddr ? shdr[i].sh_addr : min_vaddr;
      max_vaddr = shdr[i].sh_addr + shdr[i].sh_size > max_vaddr ?
	shdr[i].sh_addr + shdr[i].sh_size : max_vaddr;
    }
  }
  fprintf(stderr, "_elf: min_vaddr 0x%08x\n", min_vaddr);
  fprintf(stderr, "_elf: max_vaddr 0x%08x\n", max_vaddr);
  const size_t mem_size = max_vaddr - min_vaddr;
  elf->load = mmu_allocate_raw(mmu, mem_size);
  assert(elf->load);
  fprintf(stderr, "_elf: size: %zu alloced at 0x%08x\n", mem_size, elf->load);

  //  vaddr_t entry_reloc_offs = 0;
  elf->base = 0;
  //  fprintf(stderr, "_elf: hdr->e_shnum %zu\n", hdr->e_shnum);
#define RECALC_ADDR(x) (x - elf->base + elf->load)
  
  for(size_t i=0; i < hdr->e_shnum; ++i) {
    fprintf(stderr, "_elf: shdr[i].sh_type %zu name: 0x%08x\n", shdr[i].sh_type, shdr[i].sh_name);
    if(elf->base == 0 && shdr[i].sh_type == SHT_PROGBITS) {
      elf->base = shdr[i].sh_addr;
      elf->entry = hdr->e_entry - elf->base + elf->load;
      fprintf(stderr, "_elf: base 0x%08x\n", elf->base);
      fprintf(stderr, "_elf: load 0x%08x\n", elf->load);
      fprintf(stderr, "_elf: entry 0x%08x (0x%08x)\n", elf->entry, hdr->e_entry);
    }


    if (shdr[i].sh_type == SHT_INIT_ARRAY) { // 14
      assert(elf->base != 0);
      elf->init_count = shdr[i].sh_size / sizeof(uint32_t);
      elf->init_array = RECALC_ADDR(shdr[i].sh_addr);
      fprintf(stderr, "_elf: init_array: 0x%08x\n", elf->init_array);
    }
    else if (shdr[i].sh_type == SHT_FINI_ARRAY) { // 15
      assert(elf->base != 0);
      elf->fini_count = shdr[i].sh_size / sizeof(uint32_t);
      elf->fini_array = RECALC_ADDR(shdr[i].sh_addr);
      fprintf(stderr, "_elf: fini_array: 0x%08x\n", elf->fini_array);
    }
    else if(shdr[i].sh_size > 0 &&
	    shdr[i].sh_type == SHT_PROGBITS &&
	    shdr[i].sh_addr != 0) {
      fprintf(stderr, "_elf: load section from offs 0x%08x to addr 0x%08x\n", shdr[i].sh_offset, RECALC_ADDR(shdr[i].sh_addr));

      mmu_write_from(mmu,
		     elf_start + shdr[i].sh_offset,
		     RECALC_ADDR(shdr[i].sh_addr),
		     shdr[i].sh_size);
      
    }

    /* if (shdr[i].sh_type == 2) { // symbols */
    /*   char *strings = (char *)(elf_start + shdr[shdr[i].sh_link].sh_offset); */
    /*   //      entry = find_sym("_start", shdr + i, strings, elf_start); */
    /*   break; */
    /* } */
  }

  return elf;

}
