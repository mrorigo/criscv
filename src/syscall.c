
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "syscall.h"

bool handle_syscall(emulator_t *emu,
		    core_t *core,
		    uint32_t n,
		    uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
  fprintf(stderr, "syscall::%d::(0x%08x,0x%08x,0x%08x)", n, arg0, arg1, arg2);
  switch(n) {
  case SYS_exit: // exit
    fprintf(stderr, "syscall::exit\n");
    return false;
    //  case SYS_close:
    //    return true;
  case SYS_brk: {
    // arg1 is either 0, or the offset wanted
    vaddr_t res = mmu_allocate(emu->mmu, arg1, MPERM_RAW|MPERM_WRITE);
    fprintf(stderr, "syscall::brk res=0x%08x\n", res);
    core->trap_regs[15] = res;
    return true;
  }
    break;
  case SYS_write: // write
    assert(arg0 == 1);
    assert(arg2 < 256);
    char buf[256];
    bus_read_multiple(core->bus, arg1, buf, arg2, BYTE);
    fprintf(stderr, "syscall::write::stdout => '%s'", buf);
    core->trap_regs[15] = arg2; // nr of bytes written
    return true;

  case SYS_fstat: {
    assert(arg0 == 1); // stdout only pls
    struct stat buf;
    fprintf(stderr, "syscall::fstat::size => '%d'", sizeof(struct stat));
    buf.st_blksize = 512;
    buf.st_mode = 0x2190; // 0x20620 crw--w----
    buf.st_ino = 1337; // arbitrary
    buf.st_nlink = 1;
    buf.st_size = 0;
    buf.st_gid = 42;
    buf.st_uid = 3117;
    fprintf(stderr, "syscall::fstat::stdout => '%s'", buf);
    bus_write_multiple(core->bus, arg1, &buf, 112, BYTE);
    core->trap_regs[15] = 0;
    }
    return true;

  default:
    fprintf(stderr, "cpu::syscall:unknown: %d \n", n);
    return false;
  }
}
