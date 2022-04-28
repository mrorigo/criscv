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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "cpu.h"

RV32I_cpu_t *cpu_init(uint32_t initial_pc, bus_t *bus)
{
  RV32I_cpu_t *cpu = malloc(sizeof(RV32I_cpu_t));

  core_t *core0 = &cpu->cores[0];
  core0->pc     = initial_pc;
  core0->bus    = bus;
  core0->cycle  = 0;
  core0->state  = FETCH;
  core0->prefetch_pc = 0;
  core0->halted = false;
  for(size_t i=0; i < NUMREGS; i++) {
    core0->registers[i] = 0;
  }
  return cpu;
}


void fetch(core_t *core)
{
  if(core->prefetch_pc != core->pc) {
    bus_read_multiple(core->bus, core->pc, &core->instruction, 2, WORD);
    core->prefetch_pc = core->pc + 4;
  } else {
    core->instruction = core->prefetch;
  }
}

// We allow writes to ZERO, because REG_R handles this case
#define REG_W(x, y) (core->registers[x] = (y))
#define REG_R(x) (x == 0 ? 0 : core->registers[x])

#define bit(x, y, z) (((y >> x) & 1)<<z)

uint32_t slice32(const size_t from,
		 const size_t to,
		 const uint32_t value,
		 const size_t pos)
{
  const uint32_t span = (from-to+1);
  const uint32_t sliced = ((value >> to) & ((1 << span) -1));
  return pos != 0 ? sliced << (pos - span) : sliced;
}

void decode(core_t *core)
{
  //  assert(core->state == DECODE);
  register uint32_t i = core->instruction;
  register instr_t *dec = &core->decoded;

  dec->isJump = false;
  dec->writeRd = false;
  dec->writeMem = false;
  dec->readMem = false;
  
  dec->opcode = (i & 0x7f);
  dec->optype = decode_type_table[dec->opcode];

  dec->rs1 = (i >> 15) & 31;
  dec->rs1v = REG_R(dec->rs1); // TODO: Register read in decode?
  dec->rs2 = (i >> 20) & 31;
  dec->rs2v = REG_R(dec->rs2); // TODO: Register read in decode?

  dec->funct3 = (i>>12) & 7;
  dec->shamt = (i>>20) & 31;
  dec->rd = (i>>7) & 31;
  //  fprintf(stderr, "cpu::decode 0x%08x: opcode=0x%02x rs1=%x rs2=%x rd=%x f3=%1x %x\n", i, dec->opcode,
  //  	  dec->rs1, dec->rs2, dec->rd, (i & 0xf000)>>12, dec->funct3);

  dec->funct7 = 0;
    
  switch(dec->optype) {
  case R:
    dec->funct7 = (i >> 25);
    break;

  case I:
    dec->imm12 = (i>>20) & ((1<<12)-1);
    break;
    
  case S:
    dec->imm12 = (((i >>7) & 0b11111) | ((i >> 20) & 0xffffe0));
    break;
    
  case B:
    dec->imm12 = ((i & 0b1111) | ((i&1)<<11) |
		  ((i >> 31)<<12) |
		  ((i >> 20) & ((1<<10)-1)));
    break;
    
  case U:
    dec->imm20 = (i >> 12);
    break;

  case J: {
    // imm[20|10:1|11|19:12]

    // 2  1      1 1
    // 0  9      2 0 9 8
    // |  |      | | | |
    // 1  1111111111 1 11111111
    // ^  ^
    //
    //       const jImm = signExtend32(21, (bit(31, i, 20) | slice32(19, 12, i, 19) | bit(20, i, 11) | slice32(30, 21, i, 10)) << 1);

    const int32_t m = 1 << (21-1);
    const uint32_t jimm = (bit(31, i, 20) | slice32(19, 12, i, 19) | bit(20, i, 11) | slice32(30, 21, i, 10)) << 1;
    const int32_t se_imm21 = (jimm ^ m)-m; // sign extend
    dec->imm20 = se_imm21;

    /* dec->imm20   = (((i >> 19) & ((1<<12)-1)) | */
    /* 		    ((i&1) << 11) | */
    /* 		    (i & 0b11110) | */
    /* 		    ((i >> 20) & ((1<<10)-1))); */
  }
    break;

  case Unknown:
    fprintf(stderr, "cpu::decode i=0x%08x: unknown opcode=0x%08x optype=%x\n", i, dec->opcode, dec->optype);
    // TODO: Illegal instruction trap
    assert(dec->optype != Unknown);
    break;
  }

}

#define twos(x) (x >= 0x80000000) ? -(int32_t)(~x + 1) : x

void execute(core_t *core)
{
  //  assert(core->state == EXECUTE);
  instr_t *dec = &core->decoded;

  // sign extended imm12 is useful below
  const int32_t m = 1 << (12-1);
  const int32_t se_imm12 = (dec->imm12 ^ m)-m; // sign extend

  switch(dec->optype) {
  case R: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7) | ((dec->funct7 >> 5)<<11);
    dec->writeRd = true;
    switch(op) {
    case OP_ADD:  core->aluOut = dec->rs1v + dec->rs2v;                            break;
    case OP_SUB:  core->aluOut = dec->rs1v - dec->rs2v;                            break;
    case OP_SLL:  core->aluOut = (uint32_t)((int32_t)dec->rs1v) << (dec->rs2v&31); break;
    case OP_SLT:  core->aluOut = (int32_t)dec->rs1v < (int32_t)dec->rs2v ? 1 : 0;  break;
    case OP_SLTU: core->aluOut = dec->rs1v < dec->rs2v ? 1 : 0;                    break;
    case OP_XOR:  core->aluOut = dec->rs1v ^ dec->rs2v;                            break;
    case OP_SRL:  core->aluOut = dec->rs1v >> (dec->rs2v&31);                      break;
    case OP_SRA:  core->aluOut = (uint32_t)((int32_t)dec->rs1v) >> (dec->rs2v&31); break;
    case OP_OR:   core->aluOut = dec->rs1v | dec->rs2v;                            break;
    case OP_AND:  core->aluOut = dec->rs1v & dec->rs2v;                            break;
    default: assert(false); break;
    }
    break;
  } // R

  case I: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7) | ((dec->funct7 >> 5)<<11);
    dec->writeRd = true;
    // fprintf(stderr, "OPTYPE_I: rs1: %d (0x%08x)  se_imm12: 0x%08x\n", dec->rs1, dec->rs1v, se_imm12);
    switch(op) {
    case OP_JALR:
      dec->isJump = true;
      core->aluOut = core->pc + sizeof(uint32_t);
      dec->jumpTarget = dec->rs1v + se_imm12;
      // fprintf(stderr, "OP_JALR: target=0x%08x  rs1: %d (0x%08x)  se_imm12: 0x%08x\n", dec->jumpTarget, dec->rs1, dec->rs1v, se_imm12);
      break;
    case OP_LB:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = BYTE;
      core->aluOut = dec->rs2v & 0xff;
      dec->readMem = true;
      // fprintf(stderr, "OP_LB: offs=0x%08x  rs1: %d (0x%08x)  se_imm12: 0x%08x\n", dec->memOffset, dec->rs1, dec->rs1v, se_imm12);
      break;
    case OP_LH:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = HALFWORD;
      core->aluOut = dec->rs2v & 0xffff;
      dec->readMem = true;
      // fprintf(stderr, "OP_LH: offs=0x%08x  rs1: %d (0x%08x)  se_imm12: 0x%08x\n", dec->memOffset, dec->rs1, dec->rs1v, se_imm12);
      break;
    case OP_LW:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = WORD;
      core->aluOut = dec->rs2v & 0xffffffff;
      dec->readMem = true;
      // fprintf(stderr, "OP_LW: offs=0x%08x  rs1: %d (0x%08x)  se_imm12: 0x%08x\n", dec->memOffset, dec->rs1, dec->rs1v, se_imm12);
      break;
    case OP_ADDI: {
      const int32_t t_imm12 = twos((uint32_t)se_imm12);
      
      core->aluOut = (uint32_t)((int32_t)dec->rs1v + t_imm12);
      // fprintf(stderr, "OP_ADDI: rs1: %d (0x%08x) t_imm12: %d  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, t_imm12, se_imm12, core->aluOut);
      break;
    }
    case OP_SLTI:
      core->aluOut = (int32_t)dec->rs1v < (int32_t)se_imm12 ? 1 : 0;
      // fprintf(stderr, "OP_SLTI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SLTIU:
      core->aluOut = dec->rs1v < (uint32_t)se_imm12 ? 1 : 0;
      // fprintf(stderr, "OP_SLTIU: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_XORI:
      core->aluOut = dec->rs1v ^ se_imm12 ? 1 : 0;
      // fprintf(stderr, "OP_XORI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_ORI:
      core->aluOut = dec->rs1v | (se_imm12&0xfff) ? 1 : 0;
      // fprintf(stderr, "OP_ORI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_ANDI:
      core->aluOut = dec->rs1v & se_imm12 ? 1 : 0;
      // fprintf(stderr, "OP_ANDI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SLLI:
      core->aluOut = dec->rs1v << dec->shamt;
      // fprintf(stderr, "OP_SLLI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SRLI:
      core->aluOut = dec->rs1v >> dec->shamt;
      // fprintf(stderr, "OP_SRLI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SRAI:
      core->aluOut = ((int32_t)dec->rs1v) >> dec->shamt;
      // fprintf(stderr, "OP_SRAI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    default: assert(false); break;
    }
    break;
  }

  case S: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7);
    const int32_t t_imm12 = twos((uint32_t)se_imm12);
    dec->writeMem = true;
    dec->memOffset = dec->rs1v + t_imm12;
    switch(op) {
    case OP_SB: {
      dec->memAccessWidth = BYTE;
      core->aluOut = dec->rs2v & 0xff;
      break;
    }
    case OP_SH: {
      dec->memAccessWidth = HALFWORD;
      core->aluOut = dec->rs2v & 0xffff;
      break;
    }
    case OP_SW: {
      // 111111101111 01000 010 01100 0100011
      // ^            ^     ^   ^     ^
      // imm110       rs1   f3  imm40 opcode
      //                             
      dec->memAccessWidth = WORD;
      core->aluOut = dec->rs2v & 0xffffffff; // the word to store
      // fprintf(stderr, "OP_SW: offs=0x%08x  rs1: %d (0x%08x)  rs2: %d (0x%08x) imm12: 0x%03x se_imm12: 0x%08x  t_imm12: %d\n", dec->memOffset, dec->rs1, dec->rs1v, dec->rs2, dec->rs2v, dec->imm12, se_imm12, t_imm12);
      break;
    }
    default: assert(false); break;
    }
    break;
  } // S-type

  case B: {
    const uint32_t op = (dec->opcode | (dec->funct3 << 7));
    const int32_t m = 1 << (20 -1);
    const int32_t se_imm20 = (dec->imm20 ^ m) - m;
    dec->jumpTarget = core->pc + se_imm20;
    //    fprintf(stderr, "cpu::execute: B op=0x%08x  OP_BGE: 0x%08x\n", op, OP_BGE);
    switch(op) {
    case OP_BEQ:  core->aluOut = dec->rs1v == dec->rs2v ? 1 : 0;                  break;
    case OP_BNE:  core->aluOut = dec->rs1v != dec->rs2v ? 1 : 0;                  break;
    case OP_BLT:  core->aluOut = (int32_t)dec->rs1v < (int32_t)dec->rs2v ? 1 : 0; break;
    case OP_BLTU: core->aluOut = dec->rs1v < dec->rs2v ? 1 : 0;                   break;
    case OP_BGE:  core->aluOut = (int32_t)dec->rs1v > (int32_t)dec->rs2v ? 1 : 0; break;
    case OP_BGEU: core->aluOut = dec->rs1v > dec->rs2v ? 1 : 0;                   break;
    }
    dec->isJump = core->aluOut == 1;
    break;
  } // B

  case U: {
    dec->writeRd = true;
    switch(dec->opcode) {
    case OP_LUI & 0x7f:
      core->aluOut = (dec->imm20 << 12);
      break;
    case OP_AUIPC & 0x7f: {
      const int32_t m = 1 << (20 -1);
      const int32_t se_imm20 = (dec->imm20 ^ m) - m;
      core->aluOut = se_imm20 + core->pc;
      break;
    }
    default:
      assert(false);
      break;
    }
    break;
  } // U

  case J: {
    const int32_t m = 1 << (20 -1);
    const int32_t se_imm20 = (dec->imm20 ^ m) - m;
    //    fprintf(stderr, "cpu::execute J opcode=0x%02x OP_JAL=0x%02x %x\n", dec->opcode, OP_JAL & 0x7f);
    dec->isJump = true;
    dec->writeRd = true;
    switch(dec->opcode) {
    case OP_JAL & 0x7f:
      dec->jumpTarget = core->pc + se_imm20;
      break;
    default:
      assert(false == dec->opcode);
      break;
    }
    
    break;
  } // J
    
  case C: {
    //    switch(dec->funct3) {
    //    }
    assert(dec->optype != C);
    break;
  } // C

  default:
    assert(false);
    break;
  }
}

void memory_access(core_t *core)
{
  const instr_t *dec = &core->decoded;
  //  assert(core->state == MEMORY);
  if(dec->readMem) {
    core->aluOut = bus_read_single(core->bus, dec->memOffset, dec->memAccessWidth);
    //    fprintf(stderr, "memory_access::read 0x%08x => 0x%08x\n", dec->memOffset, core->aluOut);
  } else if(dec->writeMem) {
    bus_write_single(core->bus, dec->memOffset, core->aluOut, dec->memAccessWidth);
    //    fprintf(stderr, "memory_access::write 0x%08x <= 0x%08x\n", dec->memOffset, core->aluOut);
  }
}


void writeback(core_t *core)
{
  const instr_t *dec = &core->decoded;
  //  assert(core->state == WRITEBACK);
  if(dec->writeRd) {
    REG_W(core->decoded.rd, core->aluOut);
    //    fprintf(stderr, "writeback::writeRd X%d = 0x%08x\n", core->decoded.rd, core->aluOut);
  }

  if(dec->isJump) {
    core->pc = dec->jumpTarget;
  } else {
    core->pc += sizeof(uint32_t);
  }
}


void cpu_cycle(RV32I_cpu_t *cpu, uint8_t core_id)
{
  core_t *core = &cpu->cores[core_id];
  
  //  fprintf(stderr, "cpu_cycle %hhu %llu %hhu\n", core_id, core->cycle, core->state);

  switch(core->state) {
  case TRAP:
    switch(core->trap_state) {
    case ENTER:
      memcpy(core->trap_regs, core->registers, NUMREGS*sizeof(uint32_t));
      core->trap_pc = core->pc;
      core->trap_state = EXIT;
      core->state = FETCH;
      break;

    case EXIT:
      memcpy(core->registers, core->trap_regs, NUMREGS*sizeof(uint32_t));
      core->trap_state = NONE;
      core->pc = core->trap_pc;
      core->state = FETCH;
      break;
    }
    break;

  case FETCH:     fetch(core);         core->state = DECODE;    //break;
  case DECODE:    decode(core);        core->state = EXECUTE;   //break;
  case EXECUTE:   execute(core);       core->state = MEMORY;    //break;
  case MEMORY:    memory_access(core); core->state = WRITEBACK; //break;
  case WRITEBACK: writeback(core);     core->state = FETCH;
    core->cycle+=5;
    break;
  }
  /*  if(core->state == FETCH) {
    fprintf(stderr, "Cycle %llu  PC=0x%08x  Registers: ", core->cycle, core->pc);
    for(size_t i = 0; i < NUMREGS; i++) {
      fprintf(stderr, "X%zu=%08x ", i, core->registers[i]);
    }
    fprintf(stderr, "\n\n");
    }*/
}
