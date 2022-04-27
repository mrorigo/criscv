CCACHE=ccache
CC=$(CCACHE) clang
INCLUDE=`pkg-config --cflags libelf`
CFLAGS=-g -Wextra -Wall -Wpedantic -Wno-gnu-binary-literal -O3 ${INCLUDE}
#-fsanitize=address
#CCOPTS=-ggdb -Wextra -Wall -Wpedantic -O3
LD=clang
LDFLAGS=-lpthread
#-fsanitize=address


objects = memory.o bus.o cpu.o elf32.o main.o

main: $(objects) Makefile
	$(LD) $(LDFLAGS) -o main $(objects)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $*.o -c $<

clean:
	rm -f $(objects) main

check-syntax:
	$(CC) -fsyntax-only -Wall ${INCLUDE} ${CHK_SOURCES} || true
