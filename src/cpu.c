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
#include <pthread.h>

#include "cpu.h"
#include "memory.h"

RV32I_cpu_t *cpu_init(bus_t *bus, uint32_t num_cores)
{
  RV32I_cpu_t *cpu = malloc(sizeof(RV32I_cpu_t));
  memset(cpu, 0, sizeof(RV32I_cpu_t));
  cpu->bus = bus;

  return cpu;
}

core_t *core_init(RV32I_cpu_t *cpu, uint32_t core_num, uint32_t initial_pc)
{
  assert(core_num < NUMCORES);

  core_t *core = &cpu->cores[core_num];
  assert(core);

  core->id     = core_num;
  core->pc     = initial_pc;
  core->bus    = cpu->bus;
  core->cycle  = 0;
  core->state  = FETCH;
  core->prefetch_cnt = 0;
  core->halted = false;
  for(size_t i=0; i < NUMREGS; i++) {
    core->registers[i] = 0;
  }
  return core;
}

void core_dumpregs(core_t *core)
{
  fprintf(stderr, "pc=0x%08x ", core->pc);
  for(size_t i = 0 ; i < NUMREGS; i++) {
    fprintf(stderr, "X%02zu: 0x%08x ", i, core->registers[i]);
    if(i % 4 == 3) {
      fprintf(stderr, "\n");
    }
  }
}


void cause_trap(core_t *core, trap_cause_t cause) {
  core->state = TRAP;
  core->trap_state = ENTER;
  core->csr.mcause = cause;
}


void fetch(core_t *core)
{
  if(core->prefetch_cnt == 0) {
    bus_read_multiple(core->bus, core->pc, &core->instruction, PREFETCH_SIZE, WORD);
    if(core->bus->status != OK) {
      cause_trap(core, core->bus->status == READ_MISALIGNED ? INSTRUCTION_ADDR_MISALIGN : INSTRUCTION_ACCESS_FAULT);
      return;
    }
    core->prefetch_cnt = PREFETCH_SIZE-1;
  } else {
    core->instruction = core->prefetch[PREFETCH_SIZE-1-core->prefetch_cnt];
    core->prefetch_cnt--;
  }
#ifdef CPU_TRACE
  fprintf(stderr, "\ncpu::fetch pc=0x%08x, core->prefetch_cnt(%d) instr=0x%08x\n", core->pc, core->prefetch_cnt, core->instruction);
#endif
}

// We allow writes to ZERO, because REG_R handles this case (faster)
#define REG_W(x, y) (core->registers[x] = (x == 0 ? 0 : y))
#define REG_R(x) (x == 0 ? 0 : core->registers[x])

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
  dec->funct7 = 0;

#ifdef CPU_TRACE
  fprintf(stderr, "cpu::decode rs1=%hhu(0x%08x)  rs2=%hhu  rd=%hhu\n", dec->rs1, dec->rs1v, dec->rs2, dec->rd);
#endif

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
    
  case B: {
    const uint32_t imm12 = (i >> 31) & 1;
    const uint32_t imm105 = (i >> 25) & 0b111111;
    const uint32_t imm41 = (i >> 8) & 0xf;
    const uint32_t imm11 = (i >> 7) & 1;
    const uint32_t bimm = (imm12 << 12) | (imm105 << 5) | (imm41 << 1) | (imm11 << 11);

    //imm12 is only 12 bits in struct, but we have 13 for branches, so to preserve
    //top bit, we need to shift down once, and then account for that when
    // calculating jumpTarget
    dec->imm12 = bimm>>1; 
    break;
  }
    
  case U:
    dec->imm20 = (i >> 12) & 0xfffff;
    break;

  case J: {
    const uint32_t  imm20 = (i >> 31) & 0b1;
    const uint32_t  imm101 = (i >> 21) & 0b1111111111;
    const uint32_t  imm11 = (i >> 20) & 0b1;
    const uint32_t  imm1912 = (i >> 12) & 0b11111111;

    const int32_t imm =
      (imm20 << 20) |
      (imm101 << 1) |
      (imm11 << 11) |
      (imm1912 << 12);
    dec->imm20 = ((imm) << 11) >> 12;
  }
    break;

  case Unknown:
    fprintf(stderr, "cpu:%d:decode i=0x%08x: unknown opcode=0x%08x optype=%x, pc=0x%08x\n", core->id, i, dec->opcode, dec->optype, core->pc);

    // TODO: Illegal instruction trap
    cause_trap(core, ILLEGAL_INSTRUCTION);
    break;
  }

}


#define twos(x) (x >= 0x80000000) ? -(int32_t)(~x + 1) : x

int add_wrap(int32_t x, int32_t y) {
  int32_t res;
  __builtin_add_overflow(x, y, &res);
  return res;
}

void execute(core_t *core)
{
  instr_t *dec = &core->decoded;

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
    default:
      
      cause_trap(core, ILLEGAL_INSTRUCTION);

      assert(false);
      break;
    }
    break;
  } // R

  case I: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7) | ((dec->funct7 >> 5)<<11);
    const int32_t se_imm12 = (dec->imm12<<20)>>20; // sign extend
    dec->writeRd = true;
#ifdef CPU_TRACE
    fprintf(stderr, "cpu::execute I-type, imm12/se=0x%08x/0x%08x\n", dec->imm12,se_imm12);
#endif

    switch(op) {
    case OP_JALR:
      dec->isJump = true;
      core->aluOut = core->pc + sizeof(uint32_t);
#ifdef CPU_TRACE
      fprintf(stderr, "cpu::execute JALR, rs1=X%02d (0x%08x)\n", dec->rs1, dec->rs1v);
#endif
      dec->jumpTarget = dec->rs1v + se_imm12;
      break;
    case OP_LB:
    case OP_LBU:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = BYTE;
      core->aluOut = dec->rs2v & 0xff;
      dec->readMem = true;
      break;
    case OP_LHU:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = HALFWORD;
      core->aluOut = dec->rs2v & 0xffff;
      dec->readMem = true;
      break;
    case OP_LH:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = HALFWORD;
      core->aluOut = ((int32_t)(dec->rs2v & 0xffff)<<16)>>16;
      dec->readMem = true;
      break;
    case OP_LW:
      dec->memOffset = dec->rs1v + se_imm12;
      dec->memAccessWidth = WORD;
      core->aluOut = dec->rs2v & 0xffffffff;
      dec->readMem = true;
      #ifdef CPU_TRACE
      fprintf(stderr, "cpu::execute OP_LW imm12/se: 0x%08x/0x%08x  memOffset; 0x%08x\n", dec->imm12, se_imm12, dec->memOffset);
      #endif
      break;
    case OP_ADDI: {
      //      const int32_t t_imm12 = twos((uint32_t)se_imm12);
      //      const int32_t t_imm12 = ((uint32_t)se_imm12);
      core->aluOut = add_wrap((int32_t)dec->rs1v, (int32_t)se_imm12);
      #ifdef CPU_TRACE
      fprintf(stderr, "cpu::execute ADDI  imm12/se: 0x%08x/0x%08x => 0x%08x\n", dec->imm12, se_imm12, core->aluOut);
      #endif
      break;
    }
    case OP_SLTI:  core->aluOut = (int32_t)dec->rs1v < (int32_t)se_imm12 ? 1 : 0;  break;
    case OP_SLTIU: core->aluOut = dec->rs1v < (uint32_t)se_imm12 ? 1 : 0;          break;
    case OP_XORI:  core->aluOut = dec->rs1v ^ se_imm12;			           break;
    case OP_ORI:   core->aluOut = dec->rs1v | (se_imm12&0xfff);		           break;
    case OP_ANDI:  core->aluOut = dec->rs1v & se_imm12;			           break;
    case OP_SLLI:  core->aluOut = dec->rs1v << dec->shamt;			   break;
    case OP_SRLI:  core->aluOut = dec->rs1v >> dec->shamt;			   break;
    case OP_SRAI:  core->aluOut = ((int32_t)dec->rs1v) >> dec->shamt;		   break;
    default: assert(false); break;
    }
    break;
  }

  case S: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7);
    const int32_t se_imm12 = (dec->imm12<<20)>>20;
    const int32_t t_imm12 = twos((uint32_t)se_imm12);
    dec->writeMem = true;
    dec->memOffset = dec->rs1v + t_imm12;

#ifdef CPU_TRACE
    fprintf(stderr, "cpu::execute S-type memOffset=0x%08x\n", dec->memOffset);
#endif

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
      dec->memAccessWidth = WORD;
      core->aluOut = dec->rs2v & 0xffffffff; // the word to store
      break;
    }
    default: assert(false); break;
    }
    break;
  } // S

  case B: {
    const uint32_t op = (dec->opcode | (dec->funct3 << 7));
    const int32_t se_imm12 = (int32_t)((dec->imm12<<20)>>19);
    #ifdef CPU_TRACE
    fprintf(stderr, "cpu::execute B-type: func3=0x%1x, imm12/se: 0x%04x/0x%05x\n", dec->funct3, dec->imm12, se_imm12);
    #endif
    dec->jumpTarget = core->pc + se_imm12;
    switch(op) {
    case OP_BEQ:  core->aluOut = dec->rs1v == dec->rs2v ? 1 : 0;                  break;
    case OP_BNE:  core->aluOut = dec->rs1v != dec->rs2v ? 1 : 0;                  break;
    case OP_BLT:  core->aluOut = (int32_t)dec->rs1v < (int32_t)dec->rs2v ? 1 : 0; break;
    case OP_BLTU: core->aluOut = dec->rs1v < dec->rs2v ? 1 : 0;                   break;
    case OP_BGE:  core->aluOut = (int32_t)dec->rs1v >= (int32_t)dec->rs2v ? 1 : 0; break;
    case OP_BGEU: core->aluOut = dec->rs1v >= dec->rs2v ? 1 : 0;                   break;
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
      core->aluOut = (se_imm20<<12) + core->pc;
#ifdef CPU_TRACE
      fprintf(stderr, "cpu::exec AUIPC  pc=0x%08x imm20/se: 0x%08x 0x%08x => 0x%08x\n",
	      core->pc, dec->imm20, se_imm20, core->aluOut);
#endif
      break;
    }
    default:
      assert(false);
      break;
    }
    break;
  } // U

  case J: {
    dec->isJump = true;
    dec->writeRd = true;
    switch(dec->opcode) {
    case OP_JAL & 0x7f: {
      //      const uint32_t se_imm21 = dec->imm20; //(int32_t)((dec->imm20<<12)>>11);
      //      dec->jumpTarget = core->pc + (dec->imm20<<1);

      const int32_t m = 1 << (20 -1);
      const int32_t se_imm20 = (dec->imm20 ^ m) - m;
      dec->jumpTarget = (se_imm20<<1) + core->pc;

      core->aluOut = core->pc + sizeof(vaddr_t);
#ifdef CPU_TRACE
      fprintf(stderr, "JAL jumpTarget: 0x%08x imm20/se: 0x%08x/0x%08x \n", dec->jumpTarget, dec->imm20, se_imm20);
#endif
      break;
    }
    default:
      assert(false == dec->opcode);
      break;
    }
    
    break;
  } // J
    
  case C: { // System instruction
    switch(dec->opcode) {
    case OP_ECALL: {
      cause_trap(core, ENV_CALL_UMODE);
      break;
    }
    default:
    //    switch(dec->funct3) {
    //    }
      fprintf(stderr, "opcode: 0x%08x\n", dec->opcode);
      assert(dec->optype != C);
      break;
    }
    break;
  } // C

  case Unknown:
    // Illegal instruction trap
    cause_trap(core, ILLEGAL_INSTRUCTION);

    //    assert(false);
    break;
  }
}

void memory_access(core_t *core)
{
  const instr_t *dec = &core->decoded;
  if(dec->readMem) {
#ifdef MEM_TRACE
    fprintf(stderr, "cpu::memory_access::readMem at 0x%08x (%hhu) => ", dec->memOffset, dec->memAccessWidth);
#endif
    core->aluOut = bus_read_single(core->bus, dec->memOffset, dec->memAccessWidth);
    if(core->bus->status != OK) {
#ifdef MEM_TRACE
      fprintf(stderr, " ERROR\n");
#endif
      switch(core->bus->status) {
      case READ_MISALIGNED: cause_trap(core, LOAD_ADDR_MISALIGNED); return;
      case ADDRESS_NOT_FOUND: cause_trap(core, LOAD_ACCESS_FAULT); return;
      default: cause_trap(core, LOAD_PAGE_FAULT); return;
      }
    }
#ifdef MEM_TRACE
    fprintf(stderr, "0x%08x\n", core->aluOut);
#endif
  } else if(dec->writeMem) {
#ifdef MEM_TRACE
    fprintf(stderr, "cpu::memory_access::writeMem at 0x%08x (%hhu): 0x%08x\n", dec->memOffset, dec->memAccessWidth, core->aluOut);
#endif
    bus_write_single(core->bus, dec->memOffset, core->aluOut, dec->memAccessWidth);
    if(core->bus->status != OK) {
      switch(core->bus->status) {
      case WRITE_MISALIGNED: cause_trap(core, STORE_ADDR_MISALIGNED); return;
      case ADDRESS_NOT_FOUND: cause_trap(core, STORE_ACCESS_FAULT); return;
      default: cause_trap(core, STORE_PAGE_FAULT); return;
      }
    }
  }
}


void writeback(core_t *core)
{
  const instr_t *dec = &core->decoded;
  //  assert(core->state == WRITEBACK);
  if(dec->writeRd) {
    REG_W(core->decoded.rd, core->aluOut);
  }

  if(dec->isJump) {
#ifdef CPU_TRACE
    fprintf(stderr, "cpu::writeback jumptarget=0x%08x\n", dec->jumpTarget);
#endif
    core->pc = dec->jumpTarget;
    core->prefetch_cnt = 0; // flush prefetch cache
  } else {
    core->pc += sizeof(uint32_t);
  }

#ifdef CPU_TRACE
  core_dumpregs(core);
#endif
}

void trap(core_t *core)
{
#ifdef CPU_TRACE
  fprintf(stderr, "cpu::cycle: TRAP @ 0x%08x trap_state=0x%2d cause=0x%02x\n", core->pc, core->trap_state, core->csr.mcause);
#endif
  switch(core->trap_state) {
  case ENTER:
    memcpy(core->trap_regs, core->registers, NUMREGS*sizeof(uint32_t));
    core->prefetch_cnt = 0;  // flush prefetch cache, since pc changes
    core->trap_pc = core->pc;
    core->pc = core->csr.mepc = core->pc;
    core->state = TRAP;
    core->trap_state = HANDLE;
    break;

  case HANDLE:
    core->trap_state = EXIT;
    break;

  case EXIT:
    memcpy(core->registers, core->trap_regs, NUMREGS*sizeof(uint32_t));

    core->pc = core->csr.mepc + 4;

    core->trap_state = NONE;
    core->state = FETCH;
    // flush prefetch cache (might not be necessary)
    core->prefetch_cnt = 0;
    break;
  }
}


#define _stage(stage, next) stage(core); \
  core->cycle++; \
  core->state = core->state != TRAP ? next : core->state;

void core_cycle(core_t *core)
{
  //  fprintf(stderr, "core %d: pc=0x%08x\n", core->id, core->pc);

  switch(core->state) {
  case TRAP:      _stage(trap, FETCH);               break;
  case FETCH:     _stage(fetch, DECODE);             break;
  case DECODE:    _stage(decode, EXECUTE);           break;
  case EXECUTE:   _stage(execute, MEMORY);           break;
  case MEMORY:    _stage(memory_access, WRITEBACK);  break;
  case WRITEBACK: _stage(writeback, FETCH);          break;
  }

}
#undef _stage
