#ifndef __CONFIG_H__
#define __CONFIG_H__

#define PREFETCH_SIZE 8
#define NUMCORES 4

#define RAM_SIZE 1024 * 1000 * 4 // 4mb
#define ROM_SIZE 1024            // 1kb

#define ROM_START 0x10000000
#define RAM_START 0x20000000

#define STACK_SIZE (1<<12)  // 16kb is enough for everyone

#define STACK_TOP ((RAM_START + RAM_SIZE) - (STACK_SIZE))


#endif
