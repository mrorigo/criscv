CCACHE=ccache
CC=$(CCACHE) clang
CCOPTS=-fsanitize=address -g -Wextra -Wall -Wpedantic -Wno-gnu-binary-literal -O0
#CCOPTS=-ggdb -Wextra -Wall -Wpedantic -O3
LD=gcc
LDFLAGS=-lpthread -fsanitize=address


objects = memory.o bus.o cpu.o main.o

main: $(objects) Makefile
	$(LD) $(LDFLAGS) -o main $(objects)

%.o: %.c %.h
	$(CC) $(CCOPTS) -o $*.o -c $<

clean:
	rm -f $(objects) main
