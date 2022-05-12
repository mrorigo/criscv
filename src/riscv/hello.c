#include <string.h>
/**
   00010074 <_start>:
   10074:	fe010113          	addi	x2,x2,-32
   10078:	00812e23          	sw	x8,28(x2)
   1007c:	02010413          	addi	x8,x2,32
   10080:	00d00793          	addi	x15,x0,13
   10084:	fef42623          	sw	x15,-20(x8)
   10088:	fe042423          	sw	x0,-24(x8)
   1008c:	0200006f          	jal	x0,100ac <_start+0x38>
   10090:	fec42703          	lw	x14,-20(x8)
   10094:	fe842783          	lw	x15,-24(x8)
   10098:	00f707b3          	add	x15,x14,x15
   1009c:	fef42623          	sw	x15,-20(x8)
   100a0:	fe842783          	lw	x15,-24(x8)
   100a4:	00178793          	addi	x15,x15,1
   100a8:	fef42423          	sw	x15,-24(x8)
   100ac:	fe842703          	lw	x14,-24(x8)
   100b0:	00900793          	addi	x15,x0,9
   100b4:	fce7dee3          	bge	x15,x14,10090 <_start+0x1c>
   100b8:	fec42783          	lw	x15,-20(x8)
   100bc:	fef42223          	sw	x15,-28(x8)
   100c0:	fe442783          	lw	x15,-28(x8)
   100c4:	4037d793          	srai	x15,x15,0x3
   100c8:	fef42023          	sw	x15,-32(x8)
   100cc:	fe042783          	lw	x15,-32(x8)
   100d0:	00078513          	addi	x10,x15,0
   100d4:	01c12403          	lw	x8,28(x2)
   100d8:	02010113          	addi	x2,x2,32
   100dc:	00008067          	jalr	x0,0(x1)
*/

//struct _reent *_impure_ptr;

int _start() {
  unsigned char buf[32];

  for(int i=0; i<sizeof(buf); i++) {
    buf[i] = i;
  }

  for(int i=0; i<sizeof(buf); i++) {
    unsigned char v = buf[i];
    if(v != i) {
      return -999;
    }
  }

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
}
