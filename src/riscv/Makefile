CC=riscv64-unknown-elf-gcc
LD=riscv64-unknown-elf-gcc
OBJDUMP=riscv64-unknown-elf-objdump
LIBC=-nostdlib -nostartfiles  
#LIBC=-L../../../riscv-gnu-toolchain/newlib/riscv32-unknown-elf/newlib -lc
LIBC2=-L../../picolibc/build-rv32i-unknown-elf/newlib/ -lc
LDFLAGS=-Tlink.ld -mno-relax -march=rv32i -mabi=ilp32 $(LIBC)
CFLAGS=-march=rv32i -mabi=ilp32 -O0 -fomit-frame-pointer  $(INCLUDE)

all: hello

config_bare.h: ../config.h
	cpp -dM ../config.h|grep -v '#define\ _' >config_bare.h

hello: hello.o
	$(LD) $(LDFLAGS) -o hello hello.o

hello.o: hello.c
	$(CC) $(CFLAGS) -o hello.o -c hello.c

test1.o: test1.c
	$(CC) $(CFLAGS) -o test1.o -c test1.c

test1: test1.o
	$(CC) $(LDFLAGS) -o test1 test1.o


clean:
	rm -f hello hello.o test1 rom.bin rom.o

