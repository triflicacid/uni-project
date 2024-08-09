#include "cpu.h"

#include <string.h>

#include "constants.h"

// extract `<reg>` argument from word, return offset
static uint8_t arg_extract_reg(const cpu_t *cpu, uint64_t word, uint8_t pos) {
  return word >> pos;
}

// extract `<value>` argument from word, fetch value
static uint32_t arg_fetch_value(const cpu_t *cpu, uint64_t word, uint8_t pos) {
  // switch on indicator bits, extract data after
  uint8_t indicator = (word >> pos) & 0x3;
  uint32_t data = word >> (pos + 2);

  switch (indicator) {
    case ARG_IMM: return data;
    case ARG_MEM: return bus_load(&cpu->bus, data, 32);
    case ARG_REG: return REG(data);
    case ARG_REG_OFF: return REG(data) + (int32_t) data;
    default: return 0;
  }
}

// extract `<addr>` argument from word, return address
static uint32_t arg_extract_addr(const cpu_t *cpu, uint64_t word, uint8_t pos) {
  // switch on indicator bits, extract data after
  uint8_t indicator = (word >> pos) & 0x3;
  uint32_t data = word >> (pos + 2);

  switch (indicator) {
    case ARG_MEM: return data;
    case ARG_REG: return REG(data);
    case ARG_REG_OFF: return REG(data) + (int32_t) data;
    default: return 0;
  }
}

void cpu_init(cpu_t *cpu) {
  // set default file I/O handlers
  cpu->fp_out = stdout;
  cpu->fp_in = stdin;

  // clear and configure key registers
  memset(cpu->regs, 0, sizeof(cpu->regs));
  REG(REG_SP) = DRAM_SIZE;
  REG(REG_FP) = REG(REG_SP);

  // clear memory
  dram_clear(&cpu->bus.dram);

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " Initialising CPU... Done.\n");
#endif
}

uint64_t cpu_fetch(const cpu_t *cpu) {
  return bus_load(&cpu->bus, REG(REG_IP), 64);
}

// given three compare bits in binary form `0bXYZ` where X = eq, Y = mag, Z = zero
static char *cmp_bit_str(uint8_t bits) {
  switch (bits) {
    case CMP_NE: return "ne";
    case CMP_EQ: return "eq";
    case CMP_LT: return "lt";
    case CMP_LE: return "le";
    case CMP_Z:  return "z";
    case CMP_GT: return "gt";
    case CMP_GE: return "ge";
    default:     return "?";
  }
}

// load <reg> <value> -- load value into register
static bool exec_load(cpu_t *cpu, uint64_t inst) {
  uint8_t reg = arg_extract_reg(cpu, inst, OP_HEADER_SIZE);
  uint32_t value = arg_fetch_value(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);

  REG(reg) = value;

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " load: load value 0x%x into register %i\n", value, reg);
#endif

  return false;
}

// loadu <reg> <value> -- load value into upper register
static bool exec_load_upper(cpu_t *cpu, uint64_t inst) {
  uint8_t reg = arg_extract_reg(cpu, inst, OP_HEADER_SIZE);
  uint32_t value = arg_fetch_value(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);

  ((uint32_t *) &REG(reg))[1] = value;

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " loadu: load value 0x%x into register %i's upper half\n", value, reg);
#endif

  return false;
}

bool cpu_execute(cpu_t *cpu, uint64_t inst) {
  // extract opcode (bits 0-5)
  uint16_t opcode = inst & 0x3f;

#if DEBUG & DEBUG_CPU
  printf("inst=0x%x, opcode=0x%x \n", inst, opcode);
#endif

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
    // extract cmp bits from both the instruction and the flag register
    uint64_t bits = inst & OP_CMP_BITS;
    uint64_t flag_bits = REG(REG_FLAG) & FLAG_CMP_BITS;

#if DEBUG & DEBUG_CPU
    printf(DEBUG_STR "\tConditional test: %s (0x%x) ... %s" ANSI_RESET "\n", cmp_bit_str(bits >> OP_CMP_BITS_OFF),
      bits >> OP_CMP_BITS_OFF, bits == flag_bits ? ANSI_GREEN "PASS" : ANSI_RED "FAIL");
#endif

    // if condition does not pass, skip
    if (bits != flag_bits) {
      return false;
    }
  }

  switch (opcode) {
    case OP_LOAD: return exec_load(cpu, inst);
    case OP_LOAD_UPPER: return exec_load_upper(cpu, inst);
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
    printf(DEBUG_STR " Cycle #%i: ip=0x%x, ", counter++, REG(REG_IP));
#endif

    error = cpu_execute(cpu, inst);

    // increment instruction pointer
    REG(REG_IP) += sizeof(inst);
  }

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " Terminated after cycle %i due to %s\n", counter, error ? "execution error" : "halt bit set");
#endif
}

static char register_strings[][6] = {
  "$ip",
  "$sp",
  "$fp",
  "$flag",
  "$ret",
  "$zero"
};

void print_registers(cpu_t *cpu) {
  uint8_t i;

  // special registers
  for (i = 0; i < 6; i++) {
    fprintf(cpu->fp_out, "%-8s = 0x%x\n", register_strings[i], REG(i));
  }

  // GPRs
  for (i = 0; i < 16; i++) {
    fprintf(cpu->fp_out, "$r%02i     = %x\n", i + 1, REG(REG_GPR + i));
  }

  // PGPRs
  for (i = 0; i < 8; i++) {
    fprintf(cpu->fp_out, "$s%i      = %x\n", i + 1, REG(REG_PGPR + i));
  }
}
