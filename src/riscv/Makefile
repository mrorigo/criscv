CC=riscv64-unknown-elf-gcc
OBJDUMP=riscv64-unknown-elf-objdump
LDFLAGS=-march=rv32i -mabi=ilp32 -L../../.,/picolibc/build-rv32i-unknown-elf/newlib -nostdlib -nostartfiles
#INCLUDE=-I../../../picolibc//newlib/libc/stdio/ -I../../../picolibc//newlib/libc/include -I../../../picolibc//build-rv32i-unknown-elf/ -I.
CFLAGS=$(LDFLAGS) -O0 -march=rv32i -mabi=ilp32 -nostdlib -lgcc -nostartfiles $(INCLUDE)

all: hello

config_bare.h: ../config.h
	cpp -dM ../config.h|grep -v '#define\ _' >config_bare.h

hello: hello.o
	$(CC) $(LDFLAGS) -o hello hello.o

hello.o: hello.c
	$(CC) $(CFLAGS) -o hello.o -c hello.c

test1.o: test1.c
	$(CC) $(CFLAGS) -o test1.o -c test1.c

test1: test1.o
	$(CC) $(LDFLAGS) -o test1 test1.o


clean:
	rm -f hello hello.o test1 rom.bin rom.o

