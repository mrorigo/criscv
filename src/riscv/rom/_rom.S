
	#include "./config_bare.h"

	.text

	.org = ROM_START
	.global _start 

_start:	
	// ra=x1=Return address == HERE
	lui x1,(ROM_START>>12)

	// sp=x2=Initial stack
	lui x2,(STACK_TOP>>12)
	addi x2,x2,-4

	// gp=x3=Global pointer = RAM
	lui x3, RAM_START>>12

	// tp=x4=Thread pointer = RAM
	addi x4,x3,0

	// load entry point from ram
	lw  x31, 0(x2)

	jalr   x0, x31, 0

