CC=riscv64-unknown-elf-gcc
OBJDUMP=riscv64-unknown-elf-objdump
CFLAGS=$(LDFLAGS) -I.. -O0 -march=rv32i -mabi=ilp32 -nostdlib -lc -nostartfiles

rom: rom.o
	$(OBJDUMP) -S -d -h -Mno-aliases -Mnumeric rom.o

rom.o: _rom.S ../config_bare.h
	$(CC) $(CFLAGS) -o rom.o -c _rom.S

../config_bare.h:
	(cd ..; make config_bare.h)

