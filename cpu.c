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
  return cpu;
}


void fetch(core_t *core)
{
  assert(core->state == FETCH);
  core->instruction = bus_read(core->bus, core->pc);
  fprintf(stderr, "cpu::fetch instruction=%08x\n", core->instruction);
}

#define REG_R(x) (x == 0 ? 0 : core->registers[x])

void decode(core_t *core)
{
  assert(core->state == DECODE);
  register uint32_t i = core->instruction;
  register instr_t *dec = &core->decoded;

  dec->readMem = false;
  
  dec->opcode = (i & 0x7f);
  dec->optype = decode_type_table[dec->opcode];
  fprintf(stderr, "cpu::decode 0x%08x: opcode=0x%08x optype=%x\n", i,
	  dec->opcode, dec->optype);
  assert(dec->optype != Unknown);

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
    dec->imm12 = ((i & 0b11111) | ((i >> 20)&1));
    break;
    
  case B:
    dec->imm12 = ((i & 0b1111) | ((i&1)<<11) |
		  ((i >> 31)<<12) |
		  ((i >> 20) & ((1<<10)-1)));
    break;
    
  case U:
    dec->imm20 = (i >> 12) & 0xffff;
    break;
    
  case J:
    dec->imm20   = (((i >> 19) & ((1<<12)-1)) |
		    ((i&1) << 11) |
		    (i & 0b11110) |
		    ((i >> 20) & ((1<<10)-1)));
    break;

  case Unknown:
    // TODO: Illegal instruction trap
    assert(false);
    break;
  }

}


void execute(core_t *core)
{
  assert(core->state == EXECUTE);
  instr_t *dec = &core->decoded;
  dec->writeRd = false;
  dec->writeMem = false;
  
  switch(dec->optype) {
  case R:
    break;

  case I: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7) | ((dec->funct7 >> 5)<<11);
    const uint32_t se_imm12 = (uint32_t)((int32_t)((((uint16_t)dec->imm12)<<4)>>4));
    dec->writeRd = true;
    switch(op) {
    case OP_ADDI:  core->aluOut = dec->rs1v + se_imm12;                            break;
    case OP_SLTI:  core->aluOut = (int32_t)dec->rs1v < (int32_t)se_imm12 ? 1 : 0;  break;
    case OP_SLTIU: core->aluOut = dec->rs1v < se_imm12 ? 1 : 0;                    break;
    case OP_XORI:  core->aluOut = dec->rs1v ^ se_imm12 ? 1 : 0;                    break;
    case OP_ORI:   core->aluOut = dec->rs1v | se_imm12 ? 1 : 0;                    break;
    case OP_ANDI:  core->aluOut = dec->rs1v & se_imm12 ? 1 : 0;                    break;
    case OP_SLLI:  core->aluOut = dec->rs1v << dec->shamt;                         break;
    case OP_SRLI:  core->aluOut = dec->rs1v >> dec->shamt;                         break;
    }
    break;
  }
  case S:
    break;
  case B:
    break;
  case U:
    switch(dec->opcode) {
    case OP_LUI & 0x7f:
      core->aluOut = (dec->imm20 << 12);
      dec->writeRd = true;
      break;
    }
    break;
  case J:
    break;
  case Unknown:
    assert(false);
    break;
  }
}

void memory_access(core_t *core)
{
  instr_t *dec = &core->decoded;
  if(dec->readMem) {
    assert(false);
  }
  assert(core->state == MEMORY);
}

#define REG_W(x, y) (core->registers[x] = x == 0 ? 0 : (y))

void writeback(core_t *core)
{
  instr_t *dec = &core->decoded;
  assert(core->state == WRITEBACK);
  core->pc += sizeof(uint32_t);
  if(dec->writeRd) {
    REG_W(core->decoded.rd, core->aluOut);
    assert(false);
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
  core->cycle++;
}
