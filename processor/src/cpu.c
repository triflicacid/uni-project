#include "cpu.h"

#include <string.h>

#include "constants.h"
#include "debug.h"

// halt CPU, raise error, and return RET
#define CPU_RAISE_ERROR(RET) { \
  CLEAR_BIT(REG(REG_FLAG), FLAG_IS_RUNNING); \
  SET_BIT(REG(REG_FLAG), FLAG_IS_ERROR); \
  return RET; \
}

// set or clear zero flag given value
#define CPU_SET_Z(VALUE) \
  ((VALUE) == 0 \
    ? SET_BIT(REG(REG_FLAG), FLAG_ZERO) \
    : CLEAR_BIT(REG(REG_FLAG), FLAG_ZERO) \
  )

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

// extract `<reg>` argument from word, return offset
static uint8_t arg_extract_reg(const cpu_t *cpu, uint64_t word, uint8_t pos) {
  return word >> pos;
}

// extract `<value>` argument from word, fetch value
static uint64_t arg_fetch_value(cpu_t *cpu, uint64_t word, uint8_t pos) {
  // switch on indicator bits, extract data after
  uint8_t indicator = (word >> pos) & 0x3;
  uint64_t data = word >> (pos + 2);

  switch (indicator) {
    case ARG_IMM: return data;
    case ARG_MEM:
      if (!check_memory(cpu, data)) CPU_RAISE_ERROR(0)
      return bus_load(&cpu->bus, data, 64);
    case ARG_REG:
      if (!check_register(cpu, data)) CPU_RAISE_ERROR(0)
      return REG(data);
    case ARG_REG_OFF:
      if (!check_register(cpu, data)) CPU_RAISE_ERROR(0)
      data = REG(data) + (int32_t) data;
      if (!check_memory(cpu, data)) CPU_RAISE_ERROR(0)
      return bus_load(&cpu->bus, data, 64);
    default:
#if DEBUG & DEBUG_CPU
      printf(ERROR_STR " resolve <value>: unknown indicator bit battern 0x%x\n", indicator);
#endif
      CPU_RAISE_ERROR(0)
  }
}

// extract `<addr>` argument from word, return address
static uint32_t arg_extract_addr(cpu_t *cpu, uint64_t word, uint8_t pos) {
  // switch on indicator bits, extract data after
  uint8_t indicator = (word >> pos) & 0x3;
  uint32_t data = word >> (pos + 2);

  switch (indicator) {
    case ARG_MEM:
      if (!check_memory(cpu, data)) CPU_RAISE_ERROR(0)
      return data;
    case ARG_REG:
      if (!check_register(cpu, data)) CPU_RAISE_ERROR(0)
      return REG(data);
    case ARG_REG_OFF:
      if (!check_register(cpu, data)) CPU_RAISE_ERROR(0)
      return REG(data) + (int32_t) data;
    default:
#if DEBUG & DEBUG_CPU
      printf(ERROR_STR " resolve <value>: unknown indicator bit battern 0x%x\n", indicator);
#endif
      CPU_RAISE_ERROR(0)
  }
}

// given four compare bits in binary form `0bABBB` A = zero, BBB = cmp flag, return string repr
static char *cmp_bit_str(uint8_t bits) {
  switch (bits & 0x7) {
    case CMP_NE: return bits & 0x8 ? "ne/z" : "ne";
    case CMP_EQ: return bits & 0x8 ? "eq/z" : "eq";
    case CMP_LT: return bits & 0x8 ? "lt/z" : "lt";
    case CMP_LE: return bits & 0x8 ? "le/z" : "le";
    case CMP_NZ: return "nz"; // should never have Z set
    case CMP_GT: return bits & 0x8 ? "gt/z" : "gt";
    case CMP_GE: return bits & 0x8 ? "ge/z" : "ge";
    default: return '\0';
  }
}

// given three data type bits, return string repr
static char *datatype_bit_str(uint8_t bits) {
  switch (bits) {
    case DATATYPE_U32: return "hu";
    case DATATYPE_U64: return "u";
    case DATATYPE_S32: return "hs";
    case DATATYPE_S64: return "s";
    case DATATYPE_F: return "f";
    case DATATYPE_D: return "d";
    default: return '\0';
  }
}

// update ZERO flag based on register index
// assume register index is valid
static void update_zero_flag(cpu_t *cpu, uint8_t reg) {
  CPU_SET_Z(REG(reg));

#if DEBUG & DEBUG_FLAGS
  printf(DEBUG_STR ANSI_CYAN " zero flag" ANSI_RESET ": register %i (0x%llx) : %s\n" ANSI_RESET, reg, REG(reg),
         GET_BIT(REG(REG_FLAG), FLAG_ZERO) ? ANSI_GREEN "SET" : ANSI_RED "CLEAR");
#endif
}

// load <reg> <value> -- load value into register
static void exec_load(cpu_t *cpu, uint64_t inst) {
  // fetch register, check if in bounds
  uint8_t reg = arg_extract_reg(cpu, inst, OP_HEADER_SIZE);
  if (!check_register(cpu, reg)) CPU_RAISE_ERROR()

  // fetch and resolve value, check if OK
  uint64_t value = arg_fetch_value(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);
  if (!cpu_is_running(cpu)) return;

  // assign value to register
  REG(reg) = value;

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " load: load value 0x%llx into register %i\n", value, reg);
#endif

  update_zero_flag(cpu, reg);
}

// loadu <reg> <value> -- load value into upper register
static void exec_load_upper(cpu_t *cpu, uint64_t inst) {
  // fetch register, check if in bounds
  uint8_t reg = arg_extract_reg(cpu, inst, OP_HEADER_SIZE);
  if (!check_register(cpu, reg)) CPU_RAISE_ERROR()

  // fetch and resolve value, check if OK
  uint64_t value = arg_fetch_value(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);
  if (!cpu_is_running(cpu)) return;

  // store value in register's upper 32 bits
  ((uint32_t *) &REG(reg))[1] = value;

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " loadu: load value 0x%llx into register %i's upper half\n", value, reg);
#endif

  update_zero_flag(cpu, reg);
}

// store <addr> <reg> -- store value from register in memory
static void exec_store(cpu_t *cpu, uint64_t inst) {
  // fetch register, check if in bounds
  uint8_t reg = arg_extract_reg(cpu, inst, OP_HEADER_SIZE);
  if (!check_register(cpu, reg)) CPU_RAISE_ERROR()

  // fetch address, check if OK
  uint32_t addr = arg_extract_addr(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);
  if (!cpu_is_running(cpu)) return;

  // store in memory at address
  bus_store(&cpu->bus, addr, 64, REG(reg));

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " store: copy register %i (0x%llx) to address 0x%x\n", reg, REG(reg), addr);
#endif

  update_zero_flag(cpu, reg);
}

// calculate CMP flag (zero + 3), in local `uint8_t flag`
// Z flag depends on RHS
#define CALC_CMP_FLAG(LHS, RHS) { \
    if ((LHS) < (RHS)) flag = CMP_LT; \
    else if ((LHS) > (RHS)) flag = CMP_GT; \
    else flag = CMP_NE; \
    if ((LHS) == (RHS)) flag |= 0x1; \
    if ((RHS) == 0) flag |= 0x8; \
  }

// cmp <reg> <value> -- compare register with value
// e.g., `reg < value`
static void exec_compare(cpu_t *cpu, uint64_t inst) {
  // fetch data type bits
  uint8_t datatype = (inst >> OP_HEADER_SIZE) & 0x7;

  // fetch register, check if OK
  uint8_t reg = arg_extract_reg(cpu, inst, OP_HEADER_SIZE + DATATYPE_SIZE);
  if (!check_register(cpu, reg)) CPU_RAISE_ERROR()

  // fetch and resolve value, check if OK
  uint64_t value = arg_fetch_value(cpu, inst, OP_HEADER_SIZE + DATATYPE_SIZE + ARG_REG_SIZE);
  if (!cpu_is_running(cpu)) return;

  // deduce comparison flag depending on datatype
  uint8_t flag;

  switch (datatype) {
    case DATATYPE_U64: {
      uint64_t lhs = REG(reg);
      CALC_CMP_FLAG(lhs, value)
    }
    break;
    case DATATYPE_U32: {
      uint32_t *lhs = (uint32_t *) &REG(reg), *rhs = (uint32_t *) &value;
      CALC_CMP_FLAG(*lhs, *rhs)
    }
    break;
    case DATATYPE_S64: {
      int64_t *lhs = (int64_t *) &REG(reg), *rhs = (int64_t *) &value;
      CALC_CMP_FLAG(*lhs, *rhs)
    }
    break;
    case DATATYPE_S32: {
      int32_t *lhs = (int32_t *) &REG(reg), *rhs = (int32_t *) &value;
      CALC_CMP_FLAG(*lhs, *rhs)
    }
    break;
    case DATATYPE_F: {
      float *lhs = (float *) &REG(reg), *rhs = (float *) &value;
      CALC_CMP_FLAG(*lhs, *rhs)
    }
    break;
    case DATATYPE_D: {
      double *lhs = (double *) &REG(reg), *rhs = (double *) &value;
      CALC_CMP_FLAG(*lhs, *rhs)
    }
    break;
    default:
#if DEBUG & DEBUG_CPU
      printf(ERROR_STR " Unknown data type indicator: 0x%llx\n", datatype);
#endif
      CPU_RAISE_ERROR()
  }

  // update flag bits in register
  REG(REG_FLAG) = (REG(REG_FLAG) & ~0xf) | (flag & 0xf);


#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " cmp: datatype=%s (0x%x)\n", datatype_bit_str(datatype), datatype);
  printf(DEBUG_STR " cmp: register %i (0x%llx) vs 0x%llx = " ANSI_CYAN "%s\n" ANSI_RESET,
         reg, REG(reg), value, cmp_bit_str(flag));
#endif
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
}

bool cpu_is_running(const cpu_t *cpu) {
  return GET_BIT(REG(REG_FLAG), FLAG_IS_RUNNING);
}

bool cpu_is_error(const cpu_t *cpu) {
  return GET_BIT(REG(REG_FLAG), FLAG_IS_ERROR);
}

void cpu_stop(cpu_t *cpu) {
  CLEAR_BIT(REG(REG_FLAG), FLAG_IS_RUNNING);
}

uint64_t cpu_fetch(const cpu_t *cpu) {
  return bus_load(&cpu->bus, REG(REG_IP), 64);
}

void cpu_execute(cpu_t *cpu, uint64_t inst) {
  // extract opcode (bits 0-5)
  uint16_t opcode = inst & OPCODE_MASK;

  // switch on opcode prior to conditional test
  switch (opcode) {
    case OP_NOP:
#ifdef HALT_ON_NOP
      cpu_stop(cpu);
#endif
      return;
    default: ;
  }

  // is conditional test bit set?
  // if so, compare flag register
  if (GET_BIT(inst, BIT6)) {
    // extract cmp bits from both the instruction and the flag register
    uint64_t bits = inst & OP_CMP_BITS;
    uint64_t flag_bits = REG(REG_FLAG) & FLAG_CMP_BITS;

#if DEBUG & DEBUG_CPU
    printf(DEBUG_STR "\tConditional test: %s (0x%llx) ... ", cmp_bit_str(bits >> OP_CMP_BITS_OFF),
           bits >> OP_CMP_BITS_OFF);
#endif

    // special case for NZ test
    if (bits == CMP_NZ) {
      if (GET_BIT(REG(REG_FLAG), FLAG_ZERO)) {
#if DEBUG & DEBUG_CPU
        printf(ANSI_RED "FAIL\n");
#endif
        return;
      }
    }

    // if condition does not pass, skip
    else if (bits != flag_bits) {
#if DEBUG & DEBUG_CPU
      printf(ANSI_RED "FAIL\n");
#endif
      return;
    }

#if DEBUG & DEBUG_CPU
    printf(ANSI_GREEN "PASS\n");
#endif
  }

  switch (opcode) {
    case OP_LOAD: exec_load(cpu, inst);
      break;
    case OP_LOAD_UPPER: exec_load_upper(cpu, inst);
      break;
    case OP_STORE: exec_store(cpu, inst);
      break;
    case OP_COMPARE: exec_compare(cpu, inst);
      break;
    default:
#if DEBUG & DEBUG_CPU
      printf(ERROR_STR " Unknown opcode 0x%x (in instruction 0x%llx)\n", opcode, inst);
#endif
      cpu_stop(cpu);
  }
}

void cpu_start(cpu_t *cpu) {
  // set running bit, clear error bit
  SET_BIT(REG(REG_FLAG), FLAG_IS_RUNNING);
  CLEAR_BIT(REG(REG_FLAG), FLAG_IS_ERROR);

#if DEBUG & DEBUG_CPU
  uint32_t counter = 0;
  printf(DEBUG_STR " start cpu; commencing fetch-execute cycle...\n");
#endif

  // fetch-execute cycle until halt
  while (cpu_is_running(cpu)) {
#if DEBUG & DEBUG_CPU
    uint64_t ip = REG(REG_IP), inst = cpu->bus.dram.mem[ip];
    printf(DEBUG_STR ANSI_VIOLET " cycle #%i" ANSI_RESET ": ip=0x%llx, inst=0x%llx, opcode=0x%llx\n", counter++, ip, inst, inst & OPCODE_MASK);
#endif

    cpu_execute(cpu, cpu_fetch(cpu));

    // increment instruction pointer
    REG(REG_IP) += sizeof(uint64_t);
  }

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR ANSI_CYAN " halt bit set" ANSI_RESET "; terminating program after cycle %u\n", counter);
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
    fprintf(cpu->fp_out, "%-8s = 0x%llx\n", register_strings[i], REG(i));
  }

  // GPRs
  for (i = 0; i < 16; i++) {
    fprintf(cpu->fp_out, "$r%02i     = 0x%llx\n", i + 1, REG(REG_GPR + i));
  }

  // PGPRs
  for (i = 0; i < 8; i++) {
    fprintf(cpu->fp_out, "$s%i      = 0x%llx\n", i + 1, REG(REG_PGPR + i));
  }
}
