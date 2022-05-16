CC=riscv64-unknown-elf-gcc
LD=riscv64-unknown-elf-gcc
OBJDUMP=riscv64-unknown-elf-objdump
#LIBC=-nostdlib -nostartfiles  
LIBC=-L../../../riscv-gnu-toolchain/newlib/riscv32-unknown-elf/newlib -lc
INCLUDE=-nostdinc -I../../../riscv-gnu-toolchain/riscv-gcc/gcc/ginclude -I../../../riscv-gnu-toolchain/newlib/newlib/libc/include/ 
LDFLAGS=-Tlink.ld -mno-relax -march=rv32i -mabi=ilp32 $(LIBC)
CFLAGS=-march=rv32i -ggdb -mabi=ilp32 -O2 $(INCLUDE)

all: hello

hello: hello.o
	$(LD) $(LDFLAGS) -o hello hello.o
%.o: %.c
	$(CC) $(CFLAGS) -o $*.o -c $<

%.o: %.S
	$(CC) $(CFLAGS) -o $*.o -c $<

clean:
	rm -f hello *.o

