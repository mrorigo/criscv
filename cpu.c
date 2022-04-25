#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "cpu.h"

RV32I_t *cpu_init(uint32_t initial_pc, bus_t *bus)
{
  RV32I_t *cpu = malloc(sizeof(RV32I_t));
  core_t *core0 = &cpu->cores[0];
  core0->state = FETCH;
  core0->pc = initial_pc;
  core0->bus = bus;
  return cpu;
}

void fetch(core_t *core)
{
  assert(core->state == FETCH);
  core->instruction = bus_read(core->bus, core->pc);
}


void decode(core_t *core)
{
  assert(core->state == DECODE);
  uint32_t val = core->instruction;

  const opcode_t opcode = (val & 0x7f);
  const optype_t optype = decode_type_table[opcode];
  assert(optype != Unknown);

  switch(optype) {
  case R:
    break;

  case I:
    break;

  case S: {
    const size_t imm5 = (val >> 7) & 31;
  }
    break;

  case B: {
    const size_t imm5 = (val >> 7) & 31;
  }
    break;

  case U: {
    const size_t imm20 = (val >> 20);
    switch(opcode) {
    case OP_LUI:
      break;
    case OP_AUIPC:
      break;
    case OP_LB:
      break;
    case OP_ADDI:
      break;
    case OP_FENCE:
      break;
    case OP_SB:
      break;
    case OP_ADD:
      break;
    case OP_BEQ:
      break;
    case OP_JALR:
      break;
    case OP_ECALL:
      break;
    }
  }

  case J:
    break;

  case Unknown:
    break;
  }

}

void execute(core_t *core)
{
  assert(core->state == EXECUTE);
}

void memory_access(core_t *core)
{
  assert(core->state == MEMORY);
}

void writeback(core_t *core)
{
  assert(core->state == WRITEBACK);
  core->pc++;
}



void cpu_cycle(RV32I_t *cpu, uint8_t core_id)
{
  core_t *core = &cpu->cores[core_id];

  fprintf(stderr, "cpu_cycle %hhu %hhu\n", core_id, core->state);
  
  switch(core->state) {
  case FETCH:     fetch(core);         core->state = DECODE;    break;
  case DECODE:    decode(core);        core->state = EXECUTE;   break;
  case EXECUTE:   execute(core);       core->state = MEMORY;    break;
  case MEMORY:    memory_access(core); core->state = WRITEBACK; break;
  case WRITEBACK: writeback(core);     core->state = FETCH; break;
  }
  assert(core);
}
