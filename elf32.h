#ifndef __ELF_H__
#define __ELF_H__

#include <sys/types.h>

void *elf_load(unsigned char *elf_start, const uint32_t size);

#endif
