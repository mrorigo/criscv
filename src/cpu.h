/**

Copyright 2022 orIgo <mrorigo@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef __CPU_H__
#define __CPU_H__

#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>
#include "config.h"
#include "bus.h"
#include "csr.h"

typedef enum __attribute__((packed)) _optype_t {
  Unknown = 0,
  R = 1,
  I = 2,
  S = 3,
  B = 4,
  U = 5,
  J = 6,
  C = 7    // System (CSR*)
} optype_t;

static const optype_t decode_type_table[128] = {
  /*0000000 */ Unknown,
  /*0000001 */ Unknown,
  /*0000010 */ Unknown,
  /*0000011 = LB/LH/LW */ I,
  /*0000100 */ Unknown,
  /*0000101 */ Unknown,
  /*0000110 */ Unknown,
  /*0000111 */ Unknown,
  /*0001000 */ Unknown,
  /*0001001 */ Unknown,
  /*0001010 */ Unknown,
  /*0001011 */ Unknown,
  /*0001100 */ Unknown,
  /*0001101 */ Unknown,
  /*0001110 */ Unknown,
  /*0001111 */ Unknown,
  /*0010000 */ Unknown,
  /*0010001 */ Unknown,
  /*0010010 */ Unknown,
  /*0010011 = ADDI/SLTI/SLTIU/XORI/ORI/ANDI/SLLI/SRLI/SRAI */ I,
  /*0010100 */ Unknown,
  /*0010101 */ Unknown,
  /*0010110 */ Unknown,
  /*0010111 = AUIPC */ U,
  /*0011000 */ Unknown,
  /*0011001 */ Unknown,
  /*0011010 */ Unknown,
  /*0011011 */ Unknown,
  /*0011100 */ Unknown,
  /*0011101 */ Unknown,
  /*0011110 */ Unknown,
  /*0011111 */ Unknown,  // 32
  /*0100000 */ Unknown,
  /*0100001 */ Unknown,
  /*0100010 */ Unknown,
  /*0100011 = SB/SH/SW */ S,
  /*0100100 */ Unknown,
  /*0100101 */ Unknown,
  /*0100110 */ Unknown,
  /*0100111 */ Unknown,
  /*0101000 */ Unknown,
  /*0101001 */ Unknown,
  /*0101010 */ Unknown,
  /*0101011 */ Unknown,
  /*0101100 */ Unknown,
  /*0101101 */ Unknown,
  /*0101110 */ Unknown,
  /*0101111 */ Unknown,
  /*0110000 */ Unknown,
  /*0110001 */ Unknown,
  /*0110010 */ Unknown,
  /*0110011 XOR/XRL/SRA/OR/AND */ R,
  /*0110100 */ Unknown,
  /*0110101 */ Unknown,
  /*0110110 */ Unknown,
  /*0110111 = LUI */ U,
  /*0111000 */ Unknown,
  /*0111001 */ Unknown,
  /*0111010 */ Unknown,
  /*0111011 */ Unknown,
  /*0111100 */ Unknown,
  /*0111101 */ Unknown,
  /*0111110 */ Unknown,
  /*0111111 */ Unknown,
  /*1000000 */ Unknown,
  /*1000001 */ Unknown,
  /*1000010 */ Unknown,
  /*1000011 */ Unknown,
  /*1000100 */ Unknown,
  /*1000101 */ Unknown,
  /*1000110 */ Unknown,
  /*1000111 */ Unknown,
  /*1001000 */ Unknown,
  /*1001001 */ Unknown,
  /*1001010 */ Unknown,
  /*1001011 */ Unknown,
  /*1001100 */ Unknown,
  /*1001101 */ Unknown,
  /*1001110 */ Unknown,
  /*1001111 */ Unknown,
  /*1010000 */ Unknown,
  /*1010001 */ Unknown,
  /*1010010 */ Unknown,
  /*1010011 */ Unknown,
  /*1010100 */ Unknown,
  /*1010101 */ Unknown,
  /*1010110 */ Unknown,
  /*1010111 */ Unknown,
  /*1011000 */ Unknown,
  /*1011001 */ Unknown,
  /*1011010 */ Unknown,
  /*1011011 */ Unknown,
  /*1011100 */ Unknown,
  /*1011101 */ Unknown,
  /*1011110 */ Unknown,
  /*1011111 */ Unknown,
  /*1100000 */ Unknown,
  /*1100001 */ Unknown,
  /*1100010 */ Unknown,
  /*1100011 = BEQ/BNE/BLT/BGE/BLTU/BGEU */ B,
  /*1100100 */ Unknown,
  /*1100101 */ Unknown,
  /*1100110 */ Unknown,
  /*1100111 = JALR */ I,
  /*1101000 */ Unknown,
  /*1101001 */ Unknown,
  /*1101010 */ Unknown,
  /*1101011 */ Unknown,
  /*1101100 */ Unknown,
  /*1101101 */ Unknown,
  /*1101110 */ Unknown,
  /*1101111 = JAL */ J,
  /*1110000 */ Unknown,
  /*1110001 */ Unknown,
  /*1110010 */ Unknown,
  /*1110011 = CSR* */ C,
  /*1110100 */ Unknown,
  /*1110101 */ Unknown,
  /*1110110 */ Unknown,
  /*1110111 */ Unknown,
  /*1111000 */ Unknown,
  /*1111001 */ Unknown,
  /*1111010 */ Unknown,
  /*1111011 */ Unknown,
  /*1111100 */ Unknown,
  /*1111101 */ Unknown,
  /*1111110 */ Unknown,
  /*1111111 */ Unknown
};

// RV32I Base Instruction Set

// bits  0-6:  opcode
// bits  7-10: func3
// bit   11: bit 5 of func7
#define _OP(opcode, funct3, funct7) ((opcode | (funct3 << 7) | (funct7 << 11)))

typedef enum __attribute((packed)) _opcode_t {
  OP_NONE = 0,

    OP_LUI = _OP(0b0110111, 0b000, 0),
    OP_AUIPC = _OP(0b0010111, 0b000, 0),
    OP_JAL = _OP(0b1101111, 0b000, 0),
    OP_JALR = _OP(0b1100111, 0b000, 0),

    OP_BEQ = _OP(0b1100011, 0b000, 0),
    OP_BNE = _OP(0b1100011, 0b001, 0),
    OP_BLT = _OP(0b1100011, 0b100, 0),
    OP_BGE = _OP(0b1100011, 0b101, 0),
    OP_BLTU = _OP(0b1100011, 0b110, 0),
    OP_BGEU = _OP(0b1100011, 0b111, 0),

    OP_LB = _OP(0b0000011, 0b000, 0),
    OP_LH = _OP(0b0000011, 0b001, 0),
    OP_LW = _OP(0b0000011, 0b010, 0),
    OP_LBU = _OP(0b0000011, 0b100, 0),
    OP_LHU = _OP(0b0000011, 0b101, 0),

    OP_SB = _OP(0b0100011, 0b000, 0),
    OP_SH = _OP(0b0100011, 0b001, 0),
    OP_SW = _OP(0b0100011, 0b010, 0),

    OP_ADDI = _OP(0b0010011, 0b000, 0),
    OP_SLTI = _OP(0b0010011, 0b010, 0),
    OP_SLTIU = _OP(0b0010011, 0b011, 0),
    OP_XORI = _OP(0b0010011, 0b100, 0),
    OP_ORI = _OP(0b0010011, 0b110, 0),
    OP_ANDI = _OP(0b0010011, 0b111, 0),

    OP_SLLI = _OP(0b0010011, 0b001, 0),
    OP_SRLI = _OP(0b0010011, 0b101, 0),
    OP_SRAI = _OP(0b0010011, 0b101, 1),

    OP_ADD = _OP(0b0110011, 0b000, 0),
    OP_SUB = _OP(0b0110011, 0b000, 1),
    OP_SLL = _OP(0b0110011, 0b001, 0),
    OP_SLT = _OP(0b0110011, 0b010, 0),
    OP_SLTU = _OP(0b0110011, 0b011, 0),

    OP_XOR = _OP(0b0110011, 0b100, 0),
    OP_SRL = _OP(0b0110011, 0b101, 0),
    OP_SRA = _OP(0b0110011, 0b101, 1),
    OP_OR = _OP(0b0110011, 0b110, 0),
    OP_AND = _OP(0b0110011, 0b111, 0),

    OP_FENCE = _OP(0b0001111, 0b000, 0),
    OP_ECALL = _OP(0b1110011, 0b000, 0),


    OP_CSRRW = _OP(0b1110011, 0b001, 0),
    OP_CSRRS = _OP(0b1110011, 0b010, 0),
    OP_CSRRC = _OP(0b1110011, 0b011, 0),

    OP_CSRRWI = _OP(0b1110011, 0b101, 0),
    OP_CSRRSI = _OP(0b1110011, 0b110, 0),
    OP_CSRRCI = _OP(0b1110011, 0b111, 0),

    OP_EBREAK = _OP(0b1110011, 0b000, 0)
} opcode_t;
#undef _OP

#define NUMREGS 32

// MAYBE: Add register aliases?
typedef enum __attribute((packed)) _regn_t {
  ZERO = 0,
    X0 = 0, X1 = 1, X2, X3, X4, X5, X6, X7, X8, X9,
    X10, X11, X12, X13, X14, X15, X16, X17, X18, X19,
    X20, X21, X22, X23, X24, X25, X26, X27, X28, X29,
    X30, X31
} regn_t;
    

// Passed through the pipeline of a core
typedef struct __attribute((packed)) _instr_t {
  optype_t optype:3;
  opcode_t opcode:8;
  uint8_t  rs1:5;
  uint8_t  rs2:5;
  uint8_t  rd:5;
  uint8_t  funct7:7;
  uint8_t  funct3:3;
  uint8_t  shamt:5;
  uint8_t  imm5:5;
  uint32_t imm12:12;
  uint32_t imm20:20;
  uint32_t rs1v;
  uint32_t rs2v;

  uint32_t    memOffset;
  uint32_t    jumpTarget;

  memory_access_width_t memAccessWidth:2;
  bool        isJump:1;
  bool        writeRd:1;
  bool        readMem:1;
  bool        writeMem:1;
} instr_t;


typedef enum __attribute((packed)) _cpu_state_t {
  FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK, TRAP
} core_state_t;

typedef enum __attribute((packed)) _trap_state_t {
  NONE = 0, ENTER, EXIT
} trap_state_t;

typedef struct __attribute((packed)) _core_t {
  core_state_t state;
  
  uint32_t    instruction;
  uint32_t    prefetch[PREFETCH_SIZE-1];
  instr_t     decoded;
  
  uint32_t    pc; // pc, pcNext
  uint32_t    aluOut;

  uint32_t    registers[NUMREGS];

  bus_t       *bus;
  csr_t        csr;

  uint32_t    trap_pc;
  uint32_t    trap_regs[NUMREGS];

  uint8_t    prefetch_cnt:4; // for alignment/packing purposes
  uint8_t     id:4;
  bool        halted:1;
  trap_state_t trap_state;
  uint64_t    cycle;
} core_t;

typedef struct __attribute((packed)) _RV32I_t {
  core_t      **cores;
  pthread_t   *core_threads;
  bus_t       *bus;
} RV32I_cpu_t;

RV32I_cpu_t	*cpu_init(bus_t *, uint32_t);
core_t *	 core_init(RV32I_cpu_t *, uint32_t, uint32_t);
void		 core_cycle(core_t *);
void		 core_start(RV32I_cpu_t *, uint32_t);
void		 core_join(RV32I_cpu_t *, uint32_t);

#endif
