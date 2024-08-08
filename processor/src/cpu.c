#include "cpu.h"

#include <string.h>

#include "instructions.h"
#include "registers.h"

void cpu_init(cpu_t *cpu) {
  // set default file I/O handlers
  cpu->fp_out = stdout;
  cpu->fp_in = stdin;

  // clear and configure key registers
  memset(cpu->regs, 0, sizeof(cpu->regs));
  REG(REG_SP) = DRAM_SIZE;
  REG(REG_FP) = REG(REG_SP);

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " Initialising CPU... Done.\n");
#endif
}

uint64_t cpu_fetch(const cpu_t *cpu) {
  return bus_load(&cpu->bus, REG(REG_IP), sizeof(uint64_t));
}

bool cpu_execute(cpu_t *cpu, uint64_t inst) {
  // extract opcode (bits 0-5)
  uint16_t opcode = inst & 0x3f;

  // switch on opcode prior to conditional test
  switch (opcode) {
    case OP_NOP:
#ifdef HALT_ON_NOP
      SET_BIT(REG(REG_FLAG), FLAG_EXEC_STATUS);
#endif
      return false;
    default: ;
  }

  // is conditional test bit set?
  // if so, compare flag register
  if (GET_BIT(inst, BIT6)) {
    uint8_t bits = inst & 0x40;
    uint8_t flag_bits = REG(REG_FLAG) & FLAG_CMP_BITS;

#if DEBUG & DEBUG_CPU
    printf(DEBUG_STR "\tConditional test: %s... %s" ANSI_RESET "\n", cmp_bit_str(bits >> 4),
      bits == flag_bits ? ANSI_GREEN "PASS" : ANSI_RED "FAIL");
#endif

    // if condition does not pass, skip
    if (bits != flag_bits) {
      return false;
    }
  }

  switch (opcode) {
    // TODO
    default: goto error;
  }

  // default end: no error
  return false;

  // error handler
error:
#if DEBUG & DEBUG_CPU
    printf(ERROR_STR " Unknown opcode 0x%x (in instruction 0x%x)\n", opcode, inst);
#endif
  return true;
}

void cpu_cycle(cpu_t *cpu) {
  uint64_t inst;
  bool error = false;

#if DEBUG & DEBUG_CPU
  uint32_t counter = 0;
  printf(DEBUG_STR " Commencing fetch-execute cycle...\n");
#endif

  // fetch-execute unless encountered error or halt
  while (!(error || GET_BIT(REG(REG_FLAG), FLAG_EXEC_STATUS))) {
    inst = cpu_fetch(cpu);

#if DEBUG & DEBUG_CPU
    printf(DEBUG_STR " Cycle #%i: ip=0x%x, inst=0x%x\n", counter++, REG(REG_IP), inst);
#endif

    error = cpu_execute(cpu, inst);

    // increment instruction pointer
    REG(REG_IP) += sizeof(inst);
  }

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " Terminated after cycle %i due to %s\n", counter, error ? "execution error" : "halt bit set");
#endif
}
