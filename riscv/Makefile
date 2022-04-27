CC=riscv64-unknown-elf-gcc
LDFLAGS=-march=rv32i -mabi=ilp32 -nostartfiles -nostdlib
CFLAGS=$(LDFLAGS) -O0 -nostartfiles -nostdlib

all: hello
	riscv64-unknown-elf-objdump -S -d -h -Mno-aliases -Mnumeric hello

hello: hello.o
	$(CC) $(LDFLAGS) -o hello hello.o

hello.i: hello.c
	$(CC) $(CFLAGS) -o hello.o -c hello.c

clean:
	rm -f hello hello.o
