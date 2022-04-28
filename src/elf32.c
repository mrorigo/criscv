
#include "elf32.h"
#include <libelf.h>
//#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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

void* find_sym(const char* name, Elf32_Shdr* shdr, char* strings, const unsigned char* src)
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

void *elf_load (unsigned char *elf_start, const uint32_t size)
{
  Elf32_Ehdr      *hdr     = NULL;
  //  Elf32_Phdr      *phdr    = NULL;
  Elf32_Shdr      *shdr    = NULL;
  //  Elf32_Sym       *syms    = NULL;
  //  char            *start   = NULL;
  //  char            *taddr   = NULL;
  void            *entry   = NULL;
  int i = 0;
  //  char *exec = NULL;
  (void)size;

  hdr = (Elf32_Ehdr *) elf_start;

  //  exec = malloc(size);
  
  //  exec = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

  //  if(!exec) {
  //    perror("image_load:: error allocating memory:");
  //    return 0;
  //  }

  //  fprintf(stderr, "image_load:: ptr=0x%08x size=%08x\n", exec);
  // Start with clean memory.
  //memset(exec, 0, size);

  /* phdr = (Elf32_Phdr *)(elf_start + hdr->e_phoff); */

  /* for(i=0; i < hdr->e_phnum; ++i) { */
  /*   if(phdr[i].p_type != PT_LOAD) { */
  /*     continue; */
  /*   } */
  /*   if(phdr[i].p_filesz > phdr[i].p_memsz) { */
  /*     fprintf(stderr, "image_load:: p_filesz > p_memsz\n"); */
  /*     munmap(exec, size); */
  /*     return 0; */
  /*   } */
  /*   if(!phdr[i].p_filesz) { */
  /*     continue; */
  /*   } */

  /*   // p_filesz can be smaller than p_memsz, */
  /*   // the difference is zeroe'd out. */
  /*   start = elf_start + phdr[i].p_offset; */
  /*   taddr = phdr[i].p_vaddr + exec; */
  /*   //    memmove(taddr,start,phdr[i].p_filesz); */

  /*   /\* if(!(phdr[i].p_flags & PF_W)) { *\/ */
  /*   /\*   // Read-only. *\/ */
  /*   /\*   mprotect((unsigned char *) taddr, *\/ */
  /*   /\* 	       phdr[i].p_memsz, *\/ */
  /*   /\* 	       PROT_READ); *\/ */
  /*   /\* } *\/ */

  /*   /\* if(phdr[i].p_flags & PF_X) { *\/ */
  /*   /\*   // Executable. *\/ */
  /*   /\*   mprotect((unsigned char *) taddr, *\/ */
  /*   /\* 	       phdr[i].p_memsz, *\/ */
  /*   /\* 	       PROT_EXEC); *\/ */
  /*   /\* } *\/ */
  /* } */

  shdr = (Elf32_Shdr *)(elf_start + hdr->e_shoff);

  //  fprintf(stderr, "_elf: hdr->e_shnum %zu\n", hdr->e_shnum);
  for(i=0; i < hdr->e_shnum; ++i) {
    //    fprintf(stderr, "_elf: shdr[i].sh_type %zu\n", shdr[i].sh_type);
    if (shdr[i].sh_type == 2) {
      //syms = (Elf32_Sym*)(elf_start + shdr[i].sh_offset);
      char *strings = (char *)(elf_start + shdr[shdr[i].sh_link].sh_offset);
      entry = find_sym("_start", shdr + i, strings, elf_start);
      break;
    }
  }

  // TODO: relocation
  /* for(i=0; i < hdr->e_shnum; ++i) { */
  /*   if (shdr[i].sh_type == SHT_REL) { */
  /*     relocate(shdr + i, syms, strings, elf_start, exec); */
  /*   } */
  /* } */

  return entry;

}/* image_load */
