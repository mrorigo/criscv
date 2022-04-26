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

#include "cpu.h"

RV32I_t *cpu_init(uint32_t initial_pc, bus_t *bus)
{
  RV32I_t *cpu = malloc(sizeof(RV32I_t));

  core_t *core0 = &cpu->cores[0];
  core0->pc     = initial_pc;
  core0->bus    = bus;
  core0->cycle  = 0;
  core0->state  = FETCH;
  core0->halted = false;
  for(size_t i=0; i < NUMREGS; i++) {
    core0->registers[i] = 0;
  }
  return cpu;
}


void fetch(core_t *core)
{
  assert(core->state == FETCH);
  core->instruction = bus_read(core->bus, core->pc, WORD);
  fprintf(stderr, "cpu::fetch pc=0x%08x instruction=%08x\n",
	  core->pc, core->instruction);
}

#define REG_R(x) (x == 0 ? 0 : core->registers[x])

void decode(core_t *core)
{
  assert(core->state == DECODE);
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
  fprintf(stderr, "cpu::decode 0x%08x: rs1=%x rs2=%x rd=%x f3=%08x %x\n", i,
  	  dec->rs1, dec->rs2, dec->rd, (i & 0xf000)>>12, dec->funct3);

  dec->funct7 = 0;
    
  switch(dec->optype) {
  case R:
    dec->funct7 = (i >> 25);
    break;

  case I:
    dec->imm12 = (i>>20) & ((1<<12)-1);
    break;
    
  case S:
    dec->imm12 = (((i >>7) & 0b11111) | (i >> 20));
    break;
    
  case B:
    dec->imm12 = ((i & 0b1111) | ((i&1)<<11) |
		  ((i >> 31)<<12) |
		  ((i >> 20) & ((1<<10)-1)));
    break;
    
  case U:
    dec->imm20 = (i >> 12);
    break;
    
  case J:
    dec->imm20   = (((i >> 19) & ((1<<12)-1)) |
		    ((i&1) << 11) |
		    (i & 0b11110) |
		    ((i >> 20) & ((1<<10)-1)));
    break;

  case Unknown:
    fprintf(stderr, "cpu::decode i=0x%08x: unknown opcode=0x%08x optype=%x\n", i, dec->opcode, dec->optype);
    // TODO: Illegal instruction trap
    assert(dec->optype != Unknown);
    break;
  }

}


void execute(core_t *core)
{
  assert(core->state == EXECUTE);
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
    fprintf(stderr, "OPTYPE_I: rs1: %d (0x%08x)  se_imm12: 0x%08x\n", dec->rs1, dec->rs1v, se_imm12);
    switch(op) {
    case OP_JALR:
      core->aluOut = core->pc + 4;
      dec->isJump = true;
      dec->jumpTarget = dec->rs1v + se_imm12;
      fprintf(stderr, "OP_JALR: target=0x%08x  rs1: %d (0x%08x)  se_imm12: 0x%08x\n",
	      dec->jumpTarget, dec->rs1, dec->rs1v, se_imm12);
      break;
    case OP_LB:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = BYTE;
      core->aluOut = dec->rs2v & 0xff;
      dec->readMem = true;
      fprintf(stderr, "OP_LB: offs=0x%08x  rs1: %d (0x%08x)  se_imm12: 0x%08x\n",
	      dec->memOffset, dec->rs1, dec->rs1v, se_imm12);
      break;
    case OP_LH:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = HALFWORD;
      core->aluOut = dec->rs2v & 0xffff;
      dec->readMem = true;
      fprintf(stderr, "OP_LH: offs=0x%08x  rs1: %d (0x%08x)  se_imm12: 0x%08x\n",
	      dec->memOffset, dec->rs1, dec->rs1v, se_imm12);
      break;
    case OP_LW:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = WORD;
      core->aluOut = dec->rs2v & 0xffffffff;
      dec->readMem = true;
      fprintf(stderr, "OP_LW: offs=0x%08x  rs1: %d (0x%08x)  se_imm12: 0x%08x\n",
	      dec->memOffset, dec->rs1, dec->rs1v, se_imm12);
      break;
    case OP_ADDI:
      core->aluOut = (uint32_t)((int32_t)dec->rs1v + (int32_t)se_imm12);
      fprintf(stderr, "OP_ADDI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SLTI:
      core->aluOut = (int32_t)dec->rs1v < (int32_t)se_imm12 ? 1 : 0;
      fprintf(stderr, "OP_SLTI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SLTIU:
      core->aluOut = dec->rs1v < (uint32_t)se_imm12 ? 1 : 0;
      fprintf(stderr, "OP_SLTIU: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_XORI:
      core->aluOut = dec->rs1v ^ se_imm12 ? 1 : 0;
      fprintf(stderr, "OP_XORI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_ORI:
      core->aluOut = dec->rs1v | (se_imm12&0xfff) ? 1 : 0;
      fprintf(stderr, "OP_ORI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_ANDI:
      core->aluOut = dec->rs1v & se_imm12 ? 1 : 0;
      fprintf(stderr, "OP_ANDI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SLLI:
      core->aluOut = dec->rs1v << dec->shamt;
      fprintf(stderr, "OP_SLLI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SRLI:
      core->aluOut = dec->rs1v >> dec->shamt;
      fprintf(stderr, "OP_SRLI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    case OP_SRAI:
      core->aluOut = ((int32_t)dec->rs1v) >> dec->shamt;
      fprintf(stderr, "OP_SRAI: rs1: %d (0x%08x)  se_imm12: 0x%08x ==>  0x%08x\n", dec->rs1, dec->rs1v, se_imm12, core->aluOut);
      break;
    default: assert(false); break;
    }
    break;
  }

  case S: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7);
    const int32_t m = 1 << (20 -1);
    const int32_t se_imm20 = (dec->imm20 ^ m) - m;
    dec->writeMem = true;
    switch(op) {
    case OP_SB: {
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = BYTE;
      core->aluOut = dec->rs2v & 0xff;
      break;
    }
    case OP_SH: {
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = HALFWORD;
      core->aluOut = dec->rs2v & 0xffff;
      break;
    }
    case OP_SW: {
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = WORD;
      core->aluOut = dec->rs2v & 0xffffffff; // the word to store
      fprintf(stderr, "OP_SW: offs=0x%08x  rs1: %d (0x%08x)  rs2: %d (0x%08x)  se_imm12: 0x%06x\n",
	      dec->memOffset, dec->rs1, dec->rs1v, dec->rs2, dec->rs2v, se_imm12);
      break;
    }
    default: assert(false); break;
    }
    break;
  } // S-type

  case B:
    break;

  case U:
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

  case J: {
    const int32_t m = 1 << (20 -1);
    const int32_t se_imm20 = (dec->imm20 ^ m) - m;
    switch(dec->opcode) {
    case OP_JAL & 0x7f:
      dec->writeRd = true;
      core->aluOut = core->pc + se_imm20;
      assert(OP_JAL == 0);
      break;
    default:
      assert(false);
      break;
    }
    
    break;
  }
  default:
    assert(false);
    break;
  }
}

void memory_access(core_t *core)
{
  instr_t *dec = &core->decoded;
  if(dec->readMem) {
    core->aluOut = bus_read(core->bus, dec->memOffset, dec->memAccessWidth);
    //    fprintf(stderr, "memory_access::read 0x%08x => 0x%08x\n", dec->memOffset, core->aluOut);
  } else if(dec->writeMem) {
    bus_write(core->bus, dec->memOffset, core->aluOut, dec->memAccessWidth);
  }
  assert(core->state == MEMORY);
}

#define REG_W(x, y) (core->registers[x] = x == 0 ? 0 : (y))

void writeback(core_t *core)
{
  instr_t *dec = &core->decoded;
  assert(core->state == WRITEBACK);
  if(dec->writeRd) {
    REG_W(core->decoded.rd, core->aluOut);
    fprintf(stderr, "writeback::writeRd X%d = 0x%08x\n", core->decoded.rd, core->aluOut);
  }

  if(dec->isJump) {
    core->pc = dec->jumpTarget;
  } else {
    core->pc += sizeof(uint32_t);
  }
}


void cpu_cycle(RV32I_t *cpu, uint8_t core_id)
{
  core_t *core = &cpu->cores[core_id];
  
  fprintf(stderr, "cpu_cycle %hhu %llu %hhu\n", core_id, core->cycle, core->state);
  
  switch(core->state) {
  case FETCH:     fetch(core);         core->state = DECODE;    break;
  case DECODE:    decode(core);        core->state = EXECUTE;   break;
  case EXECUTE:   execute(core);       core->state = MEMORY;    break;
  case MEMORY:    memory_access(core); core->state = WRITEBACK; break;
  case WRITEBACK: writeback(core);     core->state = FETCH;     break;
  }
  if(core->state == FETCH) {
    fprintf(stderr, "Cycle %llu  PC=0x%08x  Registers: ", core->cycle, core->pc);
    for(size_t i = 0; i < NUMREGS; i++) {
      fprintf(stderr, "X%zu=%08x ", i, core->registers[i]);
    }
    fprintf(stderr, "\n\n");
  }
  core->cycle++;
}
