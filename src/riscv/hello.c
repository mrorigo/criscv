#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

//struct _reent *_impure_ptr;

const char *str = "string const\n";

uint32_t mstrlen(const char *s) {
  uint32_t c = 0;
  while(*s++ != 0) {
    c++;
  }
  return c;
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  printf("sizeof(struct stat): %2lu", sizeof(struct stat)+4);
  /* unsigned char buf[32]; */
  /* memset(buf, 0, sizeof(buf)); */

  /* for(int i=0; i<sizeof(buf); i++) { */
  /*   buf[i] = i; */
  /* } */

  /* for(int i=0; i<sizeof(buf); i++) { */
  /*   unsigned char v = buf[i]; */
  /*   if(v != i) { */
  /*     return -999; */
  /*   } */
  /* } */
  //close(0x1337);
  printf("argc: %d\n", argc);
  //  write(0x1, str, mstrlen(str));
  /*
  int j = 13;//strlen(argv[0]);
  int t = 19;
  while(j != 100000) {
    for(int i=0; i < 10; i++) {
      j += i | t;
    }
    int k = j;
    int y = k>>3;
    j += y;
  }

  return j;
  */  
  //  printf("hello world\n");
  return 0;
}
