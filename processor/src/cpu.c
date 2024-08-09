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

// check: memory bounds
static bool check_memory(const cpu_t *cpu, uint32_t address) {
  bool ok = address < DRAM_SIZE;

#if DEBUG & DEBUG_CPU
  if (!ok) {
    printf(ERROR_STR " SEGFAULT: address 0x%x is out of bounds\n", address);
  }
#endif

  return ok;
}

// check: register bounds
static bool check_register(const cpu_t *cpu, uint8_t reg) {
  bool ok = reg < REGISTERS;

#if DEBUG & DEBUG_CPU
  if (!ok) {
    printf(ERROR_STR " SEGFAULT: register %i is out of bounds\n", reg);
  }
#endif

  return ok;
}

// load <reg> <value> -- load value into register
static bool exec_load(cpu_t *cpu, uint64_t inst) {
  uint8_t reg = arg_extract_reg(cpu, inst, OP_HEADER_SIZE);
  if (!check_register(cpu, reg)) return true;

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
  if (!check_register(cpu, reg)) return true;

  uint32_t value = arg_fetch_value(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);

  ((uint32_t *) &REG(reg))[1] = value;

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " loadu: load value 0x%x into register %i's upper half\n", value, reg);
#endif

  return false;
}

// store <addr> <reg> -- store value from register in memory
static bool exec_store(cpu_t *cpu, uint64_t inst) {
  uint8_t reg = arg_extract_reg(cpu, inst, OP_HEADER_SIZE);
  uint32_t value = arg_fetch_value(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);

  ((uint32_t *) &REG(reg))[1] = value;

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " loadu: load value 0x%x into register %i's upper half\n", value, reg);
#endif

  return false;
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

bool cpu_is_running(const cpu_t *cpu) {
  return GET_BIT(REG(REG_FLAG), FLAG_EXEC_STATUS);
}

void cpu_stop(cpu_t *cpu) {
  CLEAR_BIT(REG(REG_FLAG), FLAG_EXEC_STATUS);
}

uint64_t cpu_fetch(const cpu_t *cpu) {
  return bus_load(&cpu->bus, REG(REG_IP), 64);
}

void cpu_execute(cpu_t *cpu, uint64_t inst) {
  // extract opcode (bits 0-5)
  uint16_t opcode = inst & 0x3f;

#if DEBUG & DEBUG_CPU
  printf("inst=0x%x, opcode=0x%x \n", inst, opcode);
#endif

  // store: should halt CPU?
  bool should_halt = false;

  // switch on opcode prior to conditional test
  switch (opcode) {
    case OP_NOP:
#ifdef HALT_ON_NOP
      cpu_stop(cpu);
#endif
      return;
    default: ;
  }

  // check to see if halt
  if (should_halt) {
    cpu_stop(cpu);
    return;
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
      cpu_stop(cpu);
      return;
    }
  }

  switch (opcode) {
    case OP_LOAD: should_halt = exec_load(cpu, inst); break;
    case OP_LOAD_UPPER: should_halt = exec_load_upper(cpu, inst); break;
    case OP_STORE: should_halt = exec_store(cpu, inst); break;
    default:
#if DEBUG & DEBUG_CPU
      printf(ERROR_STR " Unknown opcode 0x%x (in instruction 0x%x)\n", opcode, inst);
#endif
    should_halt = true;
  }

  // check to see if halt
  if (should_halt) {
    cpu_stop(cpu);
  }
}

void cpu_start(cpu_t *cpu) {
  // clear halt bit
  SET_BIT(REG(REG_FLAG), FLAG_EXEC_STATUS);

#if DEBUG & DEBUG_CPU
  uint32_t counter = 0;
  printf(DEBUG_STR " Start CPU; Commencing fetch-execute cycle...\n");
#endif

  // fetch-execute cycle until halt
  while (cpu_is_running(cpu)) {
#if DEBUG & DEBUG_CPU
    printf(DEBUG_STR " Cycle #%i: ip=0x%x, ", counter++, REG(REG_IP));
#endif

    cpu_execute(cpu, cpu_fetch(cpu));

    // increment instruction pointer
    REG(REG_IP) += sizeof(uint64_t);
  }

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " Terminated during cycle %i\n", counter);
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
