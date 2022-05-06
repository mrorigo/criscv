#ifndef __CONFIG_H__
#define __CONFIG_H__

#define PREFETCH_SIZE 4
#define NUMCORES 2

#define RAM_SIZE 1024 * 1000 * 4 // 4mb
#define ROM_SIZE 1024            // 1kb


/**
   0x20000000  - RAM START
   -- --- ---- ----- HEAP ------
   0x20e36ff0  - heap top
   0x203e6ff8  - stack bottom
   -- --- ---- ----- STACK ------
   0x203e7ff0  -
   0x203e3bf8  - stack_top
   0x203e3bfc  - argv_base (argv[0])
   0x203e7ff8  - end of argv
   0x203e7ffc  - 4 bytes entry point read by ROM
   0x203e8000  - END OF RAM
 */

#define ROM_START 0x10000000
#define RAM_START 0x20000000

#define ARGV_SIZE 1024
#define STACK_SIZE (1<<12)  // 16kb is enough for everyone
#define ARGV_START ((RAM_START + RAM_SIZE) - (8 + ARGV_SIZE))
#define STACK_TOP ((RAM_START + RAM_SIZE) - (4 + ARGV_SIZE))


#endif
