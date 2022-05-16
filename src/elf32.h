#ifndef __ELF_H__
#define __ELF_H__

#include "mmu.h"
#include <stdint.h>
 
typedef uint16_t Elf32_Half;	// Unsigned half int
typedef uint32_t Elf32_Off;	// Unsigned offset
typedef uint32_t Elf32_Addr;	// Unsigned address
typedef uint32_t Elf32_Word;	// Unsigned int
typedef int32_t  Elf32_Sword;	// Signed int

# define ELF_NIDENT	16

typedef enum _Elf32_ShType {
   SHT_NULL = 0,           // No associated section (inactive entry).
   SHT_PROGBITS = 1,       // Program-defined contents.
   SHT_SYMTAB = 2,         // Symbol table.
   SHT_STRTAB = 3,         // String table.
   SHT_RELA = 4,           // Relocation entries; explicit addends.
   SHT_HASH = 5,           // Symbol hash table.
   SHT_DYNAMIC = 6,        // Information for dynamic linking.
   SHT_NOTE = 7,           // Information about the file.
   SHT_NOBITS = 8,         // Data occupies no space in the file.
   SHT_REL = 9,            // Relocation entries; no explicit addends.
   SHT_SHLIB = 10,         // Reserved.
   SHT_DYNSYM = 11,        // Symbol table.
   SHT_INIT_ARRAY = 14,    // Pointers to initialization functions.
   SHT_FINI_ARRAY = 15,    // Pointers to termination functions.
   SHT_PREINIT_ARRAY = 16, // Pointers to pre-init functions.
   SHT_GROUP = 17,         // Section group.
   SHT_SYMTAB_SHNDX = 18,  // Indices for SHN_XINDEX entries.
} Elf32_ShType;

typedef struct {
  uint8_t	e_ident[ELF_NIDENT];
  Elf32_Half	e_type;
  Elf32_Half	e_machine;
  Elf32_Word	e_version;
  Elf32_Addr	e_entry;
  Elf32_Off	e_phoff;
  Elf32_Off	e_shoff;
  Elf32_Word	e_flags;
  Elf32_Half	e_ehsize;
  Elf32_Half	e_phentsize;
  Elf32_Half	e_phnum;
  Elf32_Half	e_shentsize;
  Elf32_Half	e_shnum;
  Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

typedef struct _Elf32_Phdr {
  Elf32_Word p_type;   // Type of segment
  Elf32_Off  p_offset;  // File offset where segment is located, in bytes
  Elf32_Addr p_vaddr;  // Virtual address of beginning of segment
  Elf32_Addr p_paddr;  // Physical address of beginning of segment (OS-specific)
  Elf32_Word p_filesz; // Num. of bytes in file image of segment (may be zero)
  Elf32_Word p_memsz;  // Num. of bytes in mem image of segment (may be zero)
  Elf32_Word p_flags;  // Segment flags
  Elf32_Word p_align;  // Segment alignment constraint
} Elf32_Phdr;

enum Elf_Ident {
  EI_MAG0		= 0, // 0x7F
  EI_MAG1		= 1, // 'E'
  EI_MAG2		= 2, // 'L'
  EI_MAG3		= 3, // 'F'
  EI_CLASS	= 4, // Architecture (32/64)
  EI_DATA		= 5, // Byte Order
  EI_VERSION	= 6, // ELF Version
  EI_OSABI	= 7, // OS Specific
  EI_ABIVERSION	= 8, // OS Specific
  EI_PAD		= 9  // Padding
};
 
# define ELFMAG0	0x7F // e_ident[EI_MAG0]
# define ELFMAG1	'E'  // e_ident[EI_MAG1]
# define ELFMAG2	'L'  // e_ident[EI_MAG2]
# define ELFMAG3	'F'  // e_ident[EI_MAG3]
 
# define ELFDATA2LSB	(1)  // Little Endian
# define ELFCLASS32	(1)  // 32-bit Architecture

enum Elf_Type {
  ET_NONE		= 0, // Unkown Type
  ET_REL		= 1, // Relocatable File
  ET_EXEC		= 2  // Executable File
};

/*
 * e_type
 */
#define ET_NONE         0
#define ET_REL          1
#define ET_EXEC         2
#define ET_DYN          3
#define ET_CORE         4
#define ET_NUM          5
#define ET_LOOS         0xfe00
#define ET_HIOS         0xfeff
#define ET_LOPROC       0xff00
#define ET_HIPROC       0xffff

/*
 * sh_flags
 */
#define SHF_WRITE     0x1
#define SHF_ALLOC     0x2
#define SHF_EXECINSTR 0x4

# define EM_386		(3)  // x86 Machine Type
# define EV_CURRENT	(1)  // ELF Current Version

typedef struct {
  Elf32_Word	st_name;
  Elf32_Addr	st_value;
  Elf32_Word	st_size;
  uint8_t	st_info;
  uint8_t	st_other;
  Elf32_Half	st_shndx;
} Elf32_Sym;

typedef struct {
  Elf32_Word	sh_name;
  Elf32_ShType	sh_type;
  Elf32_Word	sh_flags;
  Elf32_Addr	sh_addr;
  Elf32_Off	sh_offset;
  Elf32_Word	sh_size;
  Elf32_Word	sh_link;
  Elf32_Word	sh_info;
  Elf32_Word	sh_addralign;
  Elf32_Word	sh_entsize;
} Elf32_Shdr;

typedef struct _Elf32 {
  vaddr_t load;
  vaddr_t base;
  vaddr_t entry;
  vaddr_t init_array;
  uint32_t init_count;
  vaddr_t fini_array;
  uint32_t fini_count;
  vaddr_t global_pointer;
} Elf32;

Elf32 *elf_load(const unsigned char *elf_start, mmu_t *emu);

#endif
