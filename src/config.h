#ifndef __CONFIG_H__
#define __CONFIG_H__

//#define CPU_TRACE 1
//#define MEM_TRACE 1
//#define SYSCALL_TRACE 1

#define PREFETCH_SIZE 4
#define NUMCORES 1

#define RAM_SIZE 1024 * 1000 * 4 // 4mb

#define RAM_START (0x10000)

#define ARGV_SIZE 1024
#define STACK_SIZE (1<<12)  // 16kb is enough for everyone
#define ARGV_START ((RAM_START + RAM_SIZE) - (8 + ARGV_SIZE))
#define STACK_TOP ((RAM_START + RAM_SIZE) - (4 + ARGV_SIZE))


#endif
