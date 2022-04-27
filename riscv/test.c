#include <stdio.h>
#include <sys/types.h>

#define twos(x) (x >= 0x80000000) ? -(int32_t)(~x + 1) : x


int main()
{
  uint32_t i = 0x203e7bfc;
  uint32_t x = (uint32_t)0xfffff800;

  fprintf(stderr, "i: 0x%08x\n", i);
  fprintf(stderr, "x: 0x%08x\n", x);
  fprintf(stderr, "twos(x): %d\n", twos(x));

  return 0;
}
