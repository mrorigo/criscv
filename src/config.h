#ifndef __CONFIG_H__
#define __CONFIG_H__

//#define BUS_TRACE 1
//#define CPU_TRACE 1
#define CSR_TRACE 1
//#define MEM_TRACE 1
//#define SYSCALL_TRACE 1

#define PREFETCH_SIZE 8
#define NUMCORES 1

#define RAM_START	(0x10000)
#define RAM_END		(0x7ffff)
#define RAM_SIZE	(RAM_END-RAM_START)

#define VIDEO_WIDTH		640
#define VIDEO_HEIGHT		480
#define VIDEO_RAM_START		RAM_END
#define VIDEO_RAM_SIZE		(VIDEO_WIDTH*VIDEO_HEIGHT*sizeof(uint32_t))
#define VIDEO_RAM_END		(VIDEO_RAM_START+VIDEO_RAM_SIZE)

#define CSR_MMAP_BASE_ADDR	(VIDEO_RAM_END)

#define ARGV_SIZE	1024
#define STACK_SIZE	(1<<12) // 16kb is enough for everyone
#define ARGV_START	((RAM_START + RAM_SIZE) - (8 + ARGV_SIZE))
#define STACK_TOP	((RAM_START + RAM_SIZE) - (4 + ARGV_SIZE))

#endif
