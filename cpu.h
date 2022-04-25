#ifndef __CPU_H__
#define __CPU_H__

#include <sys/types.h>
#include <stdbool.h>
#include "bus.h"

#define NUMCORES 1

typedef enum __attribute__((packed)) _optype_t {
  Unknown,
  R,
  I,
  S,
  B,
  U,
  J
} optype_t;

static const optype_t decode_type_table[128] = {
  /*0000000 */ Unknown,
  /*0000001 */ Unknown,
  /*0000010 */ Unknown,
  /*0000011 */ Unknown,
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
  /*0010011 */ Unknown,
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
  /*0011111 */ Unknown,
  /*0100000 */ Unknown,
  /*0100001 */ Unknown,
  /*0100010 */ Unknown,
  /*0100011 */ Unknown,
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
  /*0110011 */ Unknown,
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
  /*1100011 */ Unknown,
  /*1100100 */ Unknown,
  /*1100101 */ Unknown,
  /*1100110 */ Unknown,
  /*1100111 = JALR */ U,
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
  /*1110011 */ Unknown,
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
typedef enum __attribute((packed)) _opcode_t {
  OP_LUI    = 0b0110111,
  OP_AUIPC  = 0b0010111,
  OP_JAL    = 0b0010111,
  OP_JALR   = 0b1100111,
  OP_BEQ    = 0b1100011,
  OP_BNE    = 0b1100011,
  OP_BLT    = 0b1100011,
  OP_BGE    = 0b1100011,
  OP_BLTU   = 0b1100011,
  OP_BGEU   = 0b1100011,
  OP_LB	    = 0b0000011,
  OP_LH	    = 0b0000011,
  OP_LW	    = 0b0000011,
  OP_LBU    = 0b0000011,
  OP_LHU    = 0b0000011,
  OP_SB	    = 0b0100011,
  OP_SH	    = 0b0100011,
  OP_SW	    = 0b0100011,
  OP_ADDI   = 0b0010011,
  OP_SLTI   = 0b0010011,
  OP_SLTIU  = 0b0010011,
  OP_XORI   = 0b0010011,
  OP_ORI    = 0b0010011,
  OP_ANDI   = 0b0010011,
  OP_SLLI   = 0b0010011,
  OP_SRLI   = 0b0010011,
  OP_SRAI   = 0b0010011,
  OP_ADD    = 0b0110011,
  OP_SUB    = 0b0110011,
  OP_SLL    = 0b0110011,
  OP_SLT    = 0b0110011,
  OP_SLTU   = 0b0110011,
  OP_XOR    = 0b0110011,
  OP_SRL    = 0b0110011,
  OP_SRA    = 0b0110011,
  OP_OR	    = 0b0110011,
  OP_AND    = 0b0110011,
  OP_FENCE  = 0b0001111,
  OP_ECALL  = 0b1110011,
  OP_EBREAK = 0b1110011
} opcode_t;

typedef enum __attribute((packed)) _cpu_state_t {
  FETCH,
  DECODE,
  EXECUTE,
  MEMORY,
  WRITEBACK
} cpu_state_t;

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
  optype_t  optype:2;
  opcode_t  opcode:8;
  uint8_t  rs1:5;
  uint8_t  rs2:5;
  uint8_t  rd: 5;
  uint8_t  funct7:7;
  uint8_t  imm5:5;
  uint32_t imm12:12;
  uint32_t imm20:20;
} instr_t;

typedef struct __attribute((packed)) _core_t {
  bus_t      *bus;
  cpu_state_t state;
  uint8_t     id:4;

  bool        halted;

  uint32_t    instruction;
  uint32_t    regs[NUMREGS];
  uint32_t    regsNext[NUMREGS];
  uint32_t    pc;
  uint32_t    pcNext;

  instr_t     decoded;
} core_t;


typedef struct __attribute((packed)) _RV32I_t {
  core_t      cores[NUMCORES];
} RV32I_t;


RV32I_t *cpu_init(uint32_t initial_pc, bus_t *bus);
void cpu_cycle(RV32I_t *cpu, uint8_t core_id);
#endif
