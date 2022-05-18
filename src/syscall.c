
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "syscall.h"

#define SYSCALL1(x) bool (x) (emulator_t *emu, core_t *core, uint32_t arg0)
#define SYSCALL2(x) bool (x) (emulator_t *emu, core_t *core, uint32_t arg0, uint32_t arg1)
#define SYSCALL3(x) bool (x) (emulator_t *emu, core_t *core, uint32_t arg0, uint32_t arg1, uint32_t arg2)
#define SYSCALL4(x) bool (x) (emulator_t *emu, core_t *core, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3)
#define SYSCALL5(x) bool (x) (emulator_t *emu, core_t *core, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4)


/**
 * File system I/O
 */
SYSCALL3(sys_read) {
  (void)emu;
  void *mem = malloc(arg2);
  core->trap_regs[10] = read(arg0, mem, arg2);
  bus_write_multiple(core->bus, arg1, mem, arg2, BYTE);
  free(mem);
#ifdef SYSCALL_TRACE
  fprintf(stderr, "syscall::read: fd=%d size=%d -> %d", arg0, arg2, core->trap_regs[10]);
#endif
  return true;
}

SYSCALL3(sys_write) {
  (void)emu;
  assert(arg0 <= 2); // can only write to stdout and stderr
  assert(arg2 < 256);
  char buf[256];
  bus_read_multiple(core->bus, arg1, buf, arg2, BYTE);
#ifdef SYSCALL_TRACE
  fprintf(stderr, "syscall::write:::%20s", buf);
#endif
  core->trap_regs[10] = write(arg0, buf, arg2);
#ifdef SYSCALL_TRACE
  fprintf(stderr, ":::%d\n", core->trap_regs[10]);
#endif
  return true;
}

SYSCALL2(sys_fstat) {
  struct stat buf;
  core->trap_regs[10] = fstat(arg0, &buf);
  
#ifdef SYSCALL_TRACE
  fprintf(stderr, "syscall::fstat::%d\n", arg0);
#endif
  // FIXME: This size is mismatched with riscv32 newlib (which is 112 bytes instead of 144)
  bus_write_multiple(core->bus, arg1, &buf, 112, BYTE);

  return true;
}


SYSCALL1(sys_close) {
  (void)emu;
#ifdef SYSCALL_TRACE
  fprintf(stderr, "syscall::close::%d\n", arg0);
#endif
  if(arg0 >= 0 && arg0 <= 2) { // close stdin/stdout/stderr, we need those ourselves
    core->trap_regs[10] = 0;
    return true;
  }
  core->trap_regs[10] = close(arg0);
  return true;
}

SYSCALL3(sys_open) {
  char buf[256];
  bus_read_string(core->bus, arg0, buf);
  if(core->bus->status != OK) {
    return false;
  }
#ifdef SYSCALL_TRACE
  fprintf(stderr, "syscall::open filename=%s => ", buf);
#endif
  core->trap_regs[10] = open(buf, arg1, arg2);
#ifdef SYSCALL_TRACE
  fprintf(stderr, "0x%08x", core->trap_regs[10]);
#endif
  return true;
}

SYSCALL3(sys_lseek) {
#ifdef SYSCALL_TRACE
  fprintf(stderr, "syscall::lseek fd=%d whence=%d ", arg0, arg2);
#endif
  core->trap_regs[10] = lseek(arg0, arg1, arg2);
  return true;
}

/**
 * Memory
 */
SYSCALL2(sys_brk) {
  // arg1 is either 0, or the offset wanted
  vaddr_t vaddr = arg0;
  const uint32_t requested_increase =  arg0 == 0 ? 0 : ( (arg0 - vaddr) + 0x100) & 0xffffff00;
  if(requested_increase > 0) {
    mmu_allocate(emu->mmu, requested_increase, MPERM_RAW|MPERM_WRITE);
  } else {
    vaddr = emu->mmu->curr_vaddr;
  }
#ifdef SYSCALL_TRACE
  fprintf(stderr, "syscall::brk 0x%08x/0x%08x  res=0x%08x  heap=0x%08x\n", arg0, requested_increase, vaddr, vaddr + requested_increase);
#endif
  core->trap_regs[10] = vaddr;
  return true;
}


bool handle_syscall(emulator_t *emu, core_t *core,
		    uint32_t n,
		    uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
   #ifdef SYSCALL_TRACE
  fprintf(stderr, "syscall::%d::(0x%08x,0x%08x,0x%08x)\n", n, arg0, arg1, arg2);
  #endif
  switch(n) {
  case SYS_exit:       fprintf(stderr, "syscall::exit()\n");       return false;
  case SYS_exit_group: fprintf(stderr, "syscall::exit_group()\n"); return false;
  case SYS_brk: return sys_brk(emu, core, arg0, arg1);
  case SYS_read: return sys_read(emu, core, arg0, arg1, arg2);
  case SYS_write: return sys_write(emu, core, arg0, arg1, arg2);
  case SYS_open:  return sys_open(emu, core, arg0, arg1, arg2);
  case SYS_close: return sys_close(emu, core, arg0);
  case SYS_lseek: return sys_lseek(emu, core, arg0, arg1, arg2);
  case SYS_fstat: return sys_fstat(emu, core, arg0, arg1);

  default:
    fprintf(stderr, "cpu::syscall:unknown: %d \n", n);
    return false;
  }
}
