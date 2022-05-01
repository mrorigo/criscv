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
  cpu->cores = malloc(sizeof(core_t *) * num_cores);
  assert(cpu->cores);
  cpu->core_threads = malloc(sizeof(pthread_t) * num_cores);
  assert(cpu->core_threads);

  return cpu;
}

core_t *core_init(RV32I_cpu_t *cpu, uint32_t core_num, uint32_t initial_pc)
{
  assert(core_num < NUMCORES);

  core_t *core = cpu->cores[core_num] = (core_t *)malloc(sizeof(core_t));
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

void cpu_thread(void *arg)
{
  core_t *core = (core_t *)arg;
  assert(core);

  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  uint64_t lastc = 0;
  uint64_t start = spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
  while(true) {
    core_cycle(core);

    //    dump_ram_rom(core->bus->mmio_devices, 100, 100, WORD);

    if(core->cycle % (uint64_t)1e8 == 0) {
      struct timespec spec;
      uint64_t cycles = core->cycle / 5;

      clock_gettime(CLOCK_REALTIME, &spec);
      uint64_t end = spec.tv_sec * 1000 + spec.tv_nsec/1.0e6;
      float mips = (float)(cycles-lastc) / ((float)(end-start));
      fprintf(stderr, "mip/s: %2f\n", (mips/1e3));
      start = end;
      lastc = cycles;
    }
  }
}

void core_start(RV32I_cpu_t *cpu, uint32_t core_num)
{
  pthread_create(&cpu->core_threads[core_num], NULL, cpu_thread, (void *)(cpu->cores[core_num]));
}

void core_join(RV32I_cpu_t *cpu, uint32_t core_num)
{
  pthread_join(cpu->core_threads[core_num], NULL);
}

void fetch(core_t *core)
{
  //  fprintf(stderr, "\ncpu::fetch pc=0x%08x, core->prefetch_cnt(%d)\n", core->pc, core->prefetch_cnt);

  
  if(core->prefetch_cnt == 0) {
    bus_read_multiple(core->bus, core->pc, &core->instruction, PREFETCH_SIZE, WORD);
    core->prefetch_cnt = PREFETCH_SIZE-1;
  } else {
    core->instruction = core->prefetch[PREFETCH_SIZE-1-core->prefetch_cnt];
    core->prefetch_cnt--;
  }

  /* fprintf(stderr, "\tinstruction: 0x%08x\n", core->instruction); */
  /* for(size_t i=0; i <PREFETCH_SIZE-1; i++) { */
  /*   fprintf(stderr, "\tprefetch[%d]: 0x%08x %s\n", i, core->prefetch[i], */
  /* 	    (core->prefetch_cnt == i) ? "*" : ""); */
  /* } */

}

// We allow writes to ZERO, because REG_R handles this case
#define REG_W(x, y) (core->registers[x] = (y))
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
  //  fprintf(stderr, "cpu::decode 0x%08x: opcode=0x%02x rs1=%x rs2=%x rd=%x f3=%1x %x\n", i, dec->opcode, dec->rs1, dec->rs2, dec->rd, (i & 0xf000)>>12, dec->funct3);

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

    const int32_t imm = (imm20 << 20) | (imm101 << 1) | (imm11 << 11) | (imm1912 << 12);
    dec->imm20 = ((imm) << 11) >> 12;
	//fprintf(stderr, "cpu::decode J imm: 0x%08x  imm20: 0x%08x\n", imm, dec->imm20);
  }
    break;

  case Unknown:
    fprintf(stderr, "cpu:%d:decode i=0x%08x: unknown opcode=0x%08x optype=%x, pc=0x%08x\n", core->id, i, dec->opcode, dec->optype, core->pc);

    // TODO: Illegal instruction trap
    core->state = TRAP;
    core->trap_state = ENTER;

    //    assert(dec->optype != Unknown);
    break;
  }

}

#define twos(x) (x >= 0x80000000) ? -(int32_t)(~x + 1) : x

void execute(core_t *core)
{
  //  assert(core->state == EXECUTE);
  instr_t *dec = &core->decoded;

  // sign extended imm12 is useful below

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
      
      core->state = TRAP;
      core->csr.mcause = 1; // TODO: defined causes
      core->trap_state = ENTER;

      assert(false);
      break;
    }
    break;
  } // R

  case I: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7) | ((dec->funct7 >> 5)<<11);
    const int32_t se_imm12 = (dec->imm12<<20)>>20; // sign extend
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
      break;
    }
    case OP_SLTI:      core->aluOut = (int32_t)dec->rs1v < (int32_t)se_imm12 ? 1 : 0;   break;
    case OP_SLTIU:     core->aluOut = dec->rs1v < (uint32_t)se_imm12 ? 1 : 0;		break;
    case OP_XORI:      core->aluOut = dec->rs1v ^ se_imm12 ? 1 : 0;			break;
    case OP_ORI:       core->aluOut = dec->rs1v | (se_imm12&0xfff) ? 1 : 0;		break;
    case OP_ANDI:      core->aluOut = dec->rs1v & se_imm12 ? 1 : 0;			break;
    case OP_SLLI:      core->aluOut = dec->rs1v << dec->shamt;				break;
    case OP_SRLI:      core->aluOut = dec->rs1v >> dec->shamt;				break;
    case OP_SRAI:      core->aluOut = ((int32_t)dec->rs1v) >> dec->shamt;		break;
    default:           assert(false);							break;
    }
    break;
  }

  case S: {
    const uint32_t op = dec->opcode | (dec->funct3 << 7);
    const int32_t se_imm12 = (dec->imm12<<20)>>20; // sign extend
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
    const int32_t se_imm12 = (int32_t)((dec->imm12<<21)>>20);
    dec->jumpTarget = core->pc + se_imm12;
    //fprintf(stderr, "cpu::execute: Branch op=0x%08x, target=0x%08x (0x%08x)  se_imm12: 0x%08x\n", op, dec->jumpTarget, dec->imm12<<1, se_imm12);
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
    //    const int32_t m = 1 << (20 -1);
    //    const int32_t se_imm20 = (dec->imm20 ^ m) - m;
    //const int32_t se_imm20 = (dec->imm20 << 11) >> 11;
    //    fprintf(stderr, "cpu::execute J opcode=0x%02x OP_JAL=0x%02x %x\n", dec->opcode, OP_JAL & 0x7f);
    dec->isJump = true;
    dec->writeRd = true;
    switch(dec->opcode) {
    case OP_JAL & 0x7f: {
      const se_imm21 = (int32_t)((dec->imm20<<13)>>12);
      dec->jumpTarget = core->pc + se_imm21;
      //      fprintf(stderr, "cpu::execute JAL jumpTarget=0x%08x imm20:0x%08x se_imm21:0x%08x\n", dec->jumpTarget, (int32_t)(dec->imm20), se_imm21);
      break;
    }
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

  case Unknown:
    // TODO: Illegal instruction trap
    core->state = TRAP;
    core->csr.mcause = 2;
    core->trap_state = ENTER;

    //    assert(false);
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
    core->prefetch_cnt = 0; // flush prefetch cache
  } else {
    core->pc += sizeof(uint32_t);
  }
}

void trap(core_t *core)
{
  switch(core->trap_state) {
  case ENTER:
    fprintf(stderr, "cpu::cycle: TRAP @ 0x%08x\n", core->pc);
    assert(false);
    memcpy(core->trap_regs, core->registers, NUMREGS*sizeof(uint32_t));
    core->prefetch_cnt = 0;  // flush prefetch cache, since pc changes
    core->trap_pc = core->pc;
    core->trap_state = EXIT;
    core->pc = 0; // TODO: Trap vector not implemented
    core->pc = core->csr.mepc = core->pc;
    core->state = FETCH;
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
  case TRAP:      trap(core);  core->cycle++;        break;
  case FETCH:     _stage(fetch, DECODE);             break;
  case DECODE:    _stage(decode, EXECUTE);           break;
  case EXECUTE:   _stage(execute, MEMORY);           break;
  case MEMORY:    _stage(memory_access, WRITEBACK);  break;
  case WRITEBACK: _stage(writeback, FETCH);          break;
  }
}
#undef _stage
