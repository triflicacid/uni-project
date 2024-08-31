#include "cpu.h"

#include <stdlib.h>
#include <string.h>

#include "arg.h"
#include "constants.h"
#include "debug.h"

// set or clear zero flag given value
#define CPU_SET_Z(VALUE) \
  ((VALUE) == 0 \
    ? SET_BIT(REG(REG_FLAG), FLAG_ZERO) \
    : CLEAR_BIT(REG(REG_FLAG), FLAG_ZERO) \
  )

// calculate CMP flag (zero + 3), in local `uint8_t flag`
// Z flag depends on RHS
#define CALC_CMP_FLAG(LHS, RHS) \
    if ((LHS) < (RHS)) flag = CMP_LT; \
    else if ((LHS) > (RHS)) flag = CMP_GT; \
    else flag = CMP_NE; \
    if ((LHS) == (RHS)) flag |= 0x1; \
    if ((RHS) == 0) flag |= 0x8;

// push an n-byte value to the stack
#define PUSH(BYTES, DATA) \
  REG(REG_SP) -= (BYTES); \
  bus_store(&cpu->bus, REG(REG_SP), (BYTES) * 4, (DATA));

// pop an n-byte value from the stack and assign to given arg
#define POP(BYTES, VAR) \
  VAR = bus_load(&cpu->bus, REG(REG_SP), (BYTES) * 4); \
  REG(REG_SP) += (BYTES);

// check: memory bounds
bool check_memory(uint32_t address) {
#if DEBUG & DEBUG_ERRS
  bool ok = address < DRAM_SIZE;

  if (!ok) {
    ERR_PRINT("SEGFAULT: address 0x%x is out of bounds\n", address)
  }

  return ok;
#else
  return address < DRAM_SIZE;
#endif
}

// check: register bounds
bool check_register(uint8_t reg) {
#if DEBUG & DEBUG_ERRS
  bool ok = reg < REGISTERS;

  if (!ok) {
    ERR_PRINT("SEGFAULT: register %i is out of bounds\n", reg)
  }

  return ok;
#else
  return reg < REGISTERS;
#endif
}

// fetch `<reg> <value> <value>`, return if OK. Index into instruction `HEADER_SIZE + offset`
// pass `is_double` to `get_arg` of value
static bool fetch_reg_val_val(cpu_t *cpu, uint64_t inst, uint8_t *reg, uint64_t *value1, uint64_t *value2,
  uint8_t offset, bool is_double) {
  *reg = get_arg_reg(cpu, inst, OP_HEADER_SIZE + offset);
  if (!CPU_RUNNING) return false;

  *value1 = get_arg_value(cpu, inst, OP_HEADER_SIZE + offset + ARG_REG_SIZE, is_double);
  if (!CPU_RUNNING) return false;

  *value2 = get_arg_value(cpu, inst, OP_HEADER_SIZE + offset + ARG_REG_SIZE + ARG_VALUE_SIZE, is_double);

  return CPU_RUNNING;
}

// given four compare bits in binary form `0bABBB` A = zero, BBB = cmp flag, return string repr
static const char *cmp_bit_str(uint8_t bits) {
  switch (bits & 0xf) {
    case CMP_Z: return "z";
    case CMP_NZ: return "nz";
    case CMP_NE: return "ne";
    case CMP_EQ: return "eq";
    case CMP_LT: return "lt";
    case CMP_LE: return "le";
    case CMP_GT: return "gt";
    case CMP_GE: return "ge";
    default: return '\0';
  }
}

// given three data type bits, return string repr
static char *datatype_bit_str(uint8_t bits) {
  switch (bits) {
    case DATATYPE_U32: return "hu";
    case DATATYPE_U64: return "u";
    case DATATYPE_S32: return "hi";
    case DATATYPE_S64: return "i";
    case DATATYPE_F: return "f";
    case DATATYPE_D: return "d";
    default: return '\0';
  }
}

// update ZERO flag based on register index
// assume register index is valid
static void update_zero_flag(cpu_t *cpu, uint8_t reg) {
  CPU_SET_Z(REG(reg));

  DEBUG_FLAGS_PRINT(DEBUG_STR ANSI_CYAN " zero flag" ANSI_RESET ": register %i (0x%llx) : %s\n" ANSI_RESET, reg, REG(reg),
         GET_BIT(REG(REG_FLAG), FLAG_ZERO) ? ANSI_GREEN "SET" : ANSI_RED "CLEAR")
}

// push stack frame
static void push_stack_frame(cpu_t *cpu) {
  // store $fp
  PUSH(8, REG(REG_FP))

  // set $fp to point to old $fp
  REG(REG_FP) = REG(REG_SP);

  // store $ip
  PUSH(8, REG(REG_IP))

  // store PGRPs ($s...)
  for (uint8_t r = REG_PGPR; r < REGISTERS; r++) {
    PUSH(8, REG(r))
  }
}

// push & restore stack frame
static void pop_stack_frame(cpu_t *cpu) {
  // move $sp to top of frame
  REG(REG_SP) = REG(REG_FP) - 8 * (REGISTERS - REG_PGPR + 1);

  // restore PGPRs
  for (uint8_t r = REGISTERS - 1; r >= REG_PGPR; r--) {
    POP(8, REG(r))
  }

  // restore old $ip
  POP(8, REG(REG_IP))

  // restore old $fp
  POP(8, REG(REG_FP))

  // pop argument count
  uint8_t argc;
  POP(4, argc)
  REG(REG_SP) += argc;
}

// load <reg> <value> -- load value into register
static void exec_load(cpu_t *cpu, uint64_t inst) {
  // fetch register, check if in bounds
  uint8_t reg = get_arg_reg(cpu, inst, OP_HEADER_SIZE);
  if (!check_register(reg)) CPU_RAISE_ERROR(ERR_REG, reg,)

  // fetch and resolve value, check if OK
  uint64_t value = get_arg_value(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE, false);
  if (!CPU_RUNNING) return;

  // assign value to register
  REG(reg) = value;

  DEBUG_CPU_PRINT(DEBUG_STR " load: load value 0x%llx into register %i\n", value, reg)
  update_zero_flag(cpu, reg);
}

// loadu <reg> <value> -- load value into upper register
static void exec_load_upper(cpu_t *cpu, uint64_t inst) {
  // fetch register, check if in bounds
  uint8_t reg = get_arg_reg(cpu, inst, OP_HEADER_SIZE);
  if (!check_register(reg)) CPU_RAISE_ERROR(ERR_REG, reg,)

  // fetch and resolve value, check if OK
  uint64_t value = get_arg_value(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE, false);
  if (!CPU_RUNNING) return;

  // store value in register's upper 32 bits
  ((uint32_t *) &REG(reg))[1] = *(uint32_t *) &value;

  DEBUG_CPU_PRINT(DEBUG_STR " loadu: load value 0x%llx into register %i's upper half\n", value, reg)
  update_zero_flag(cpu, reg);
}

// store <addr> <reg> -- store value from register in memory
static void exec_store(cpu_t *cpu, uint64_t inst) {
  // fetch register, check if in bounds
  uint8_t reg = get_arg_reg(cpu, inst, OP_HEADER_SIZE);
  if (!check_register(reg)) CPU_RAISE_ERROR(ERR_REG, reg,)

  // fetch address, check if OK
  uint32_t addr = get_arg_addr(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);
  if (!CPU_RUNNING) return;

  // store in memory at address
  bus_store(&cpu->bus, addr, 64, REG(reg));

  DEBUG_CPU_PRINT(DEBUG_STR " store: copy register %i (0x%llx) to address 0x%lx\n", reg, REG(reg), addr)
  update_zero_flag(cpu, reg);
}

// cmp <reg> <value> -- compare register with value
// e.g., `reg < value`
static void exec_compare(cpu_t *cpu, uint64_t inst) {
  // fetch data type bits
  uint8_t datatype = (inst >> OP_HEADER_SIZE) & 0x7;

  // fetch register, check if OK
  uint8_t reg = get_arg_reg(cpu, inst, OP_HEADER_SIZE + DATATYPE_SIZE);
  if (!check_register(reg)) CPU_RAISE_ERROR(ERR_REG, reg,)

  // fetch and resolve value, check if OK
  uint64_t value = get_arg_value(cpu, inst, OP_HEADER_SIZE + DATATYPE_SIZE + ARG_REG_SIZE, datatype == DATATYPE_D);
  if (!CPU_RUNNING) return;

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
      ERR_PRINT("unknown data type indicator: 0x%x\n", datatype)
      CPU_RAISE_ERROR(ERR_DATATYPE, datatype,)
  }

  // update flag bits in register
  REG(REG_FLAG) = (REG(REG_FLAG) & ~0xf) | (flag & 0xf);

  DEBUG_CPU_PRINT(DEBUG_STR " cmp: datatype=%s (0x%x)\n", datatype_bit_str(datatype), datatype);
  DEBUG_CPU_PRINT(DEBUG_STR " cmp: register %i (0x%llx) vs 0x%llx = " ANSI_CYAN "%s\n" ANSI_RESET,
         reg, REG(reg), value, cmp_bit_str(flag));
}

// not <reg> <reg>
static void exec_not(cpu_t *cpu, uint64_t inst) {
  // fetch register, check if OK
  uint8_t reg_dst = get_arg_reg(cpu, inst, OP_HEADER_SIZE);
  if (!check_register(reg_dst)) CPU_RAISE_ERROR(ERR_REG, reg_dst,)

  uint8_t reg_src = get_arg_reg(cpu, inst, OP_HEADER_SIZE + ARG_REG_SIZE);
  if (!check_register(reg_src)) CPU_RAISE_ERROR(ERR_REG, reg_src,)

  // inverse source register, update flag
  REG(reg_dst) = ~REG(reg_src);
  DEBUG_CPU_PRINT(DEBUG_STR " not: reg %i = ~0x%llx = 0x%llx\n", reg_dst, REG(reg_src), REG(reg_dst))
  update_zero_flag(cpu, reg_dst);
}

// and <reg> <reg> <value>
static void exec_and(cpu_t *cpu, uint64_t inst) {
  uint8_t reg;
  uint64_t value1, value2;

  if (!fetch_reg_val_val(cpu, inst, &reg, &value1, &value2, 0, NULL)) return;

  REG(reg) = value1 & value2;
  DEBUG_CPU_PRINT(DEBUG_STR " and: reg %i = 0x%llx & 0x%llx = 0x%llx\n", reg, value1, value2, REG(reg))
  update_zero_flag(cpu, reg);
}

// or <reg> <reg> <value>
static void exec_or(cpu_t *cpu, uint64_t inst) {
  uint8_t reg;
  uint64_t value1, value2;

  if (!fetch_reg_val_val(cpu, inst, &reg, &value1, &value2, 0, NULL)) return;

  REG(reg) = value1 | value2;
  DEBUG_CPU_PRINT(DEBUG_STR " or: reg %i = 0x%llx | 0x%llx = 0x%llx\n", reg, value1, value2, REG(reg))
  update_zero_flag(cpu, reg);
}

// xor <reg> <reg> <value>
static void exec_xor(cpu_t *cpu, uint64_t inst) {
  uint8_t reg;
  uint64_t value1, value2;

  if (!fetch_reg_val_val(cpu, inst, &reg, &value1, &value2, 0, NULL)) return;

  REG(reg) = value1 ^ value2;
  DEBUG_CPU_PRINT(DEBUG_STR " xor: reg %i = 0x%llx ^ 0x%llx = 0x%llx\n", reg, value1, value2, REG(reg))
  update_zero_flag(cpu, reg);
}

// shr <reg> <reg> <value>
static void exec_shift_left(cpu_t *cpu, uint64_t inst) {
  uint8_t reg;
  uint64_t value1, value2;

  if (!fetch_reg_val_val(cpu, inst, &reg, &value1, &value2, 0, NULL)) return;

  REG(reg) = value1 << value2;
  DEBUG_CPU_PRINT(DEBUG_STR " shl: 0x%llx << %llu = 0x%llx", value1, value2, REG(reg))
  update_zero_flag(cpu, reg);
}

// shr <reg> <reg> <value>
static void exec_shift_right(cpu_t *cpu, uint64_t inst) {
  uint8_t reg;
  uint64_t value1, value2;

  if (!fetch_reg_val_val(cpu, inst, &reg, &value1, &value2, 0, NULL)) return;

  REG(reg) = value1 >> value2;
  DEBUG_CPU_PRINT(DEBUG_STR " shl: 0x%llx << %llu = 0x%llx", value1, value2, REG(reg))
  update_zero_flag(cpu, reg);
}

// macro for arithmetic operation
#define ARITH_OPERATION(OPERATOR, INJECT) \
  uint8_t datatype = (inst >> OP_HEADER_SIZE) & 0x7;\
  uint8_t reg;\
  uint64_t value1, value2, result;\
  if (!fetch_reg_val_val(cpu, inst, &reg, &value1, &value2, DATATYPE_SIZE, datatype == DATATYPE_D))\
    return;\
  DEBUG_CPU_PRINT(DEBUG_STR " arithmetic operation: ")\
  switch (datatype) {\
    case DATATYPE_U64: {\
      int32_t *rhs = (int32_t *) &value2;\
      result = value1 OPERATOR *rhs;\
      DEBUG_CPU_PRINT("%llu " #OPERATOR " %i = %llu\n", value1, *rhs, result)\
      break;\
    }\
    case DATATYPE_U32: {\
      uint32_t *lhs = (uint32_t *) &value1;\
      int32_t *rhs = (int32_t *) &value2;\
      result = *lhs OPERATOR *rhs;\
      DEBUG_CPU_PRINT("%u " #OPERATOR " %i = %llu\n", *lhs, *rhs, result)\
    }\
    break;\
    case DATATYPE_S64: {\
      int64_t *lhs = (int64_t *) &value1;\
      int32_t *rhs = (int32_t *) &value2;\
      int64_t res = *lhs OPERATOR *rhs;\
      result = *(uint64_t *) &res;\
      DEBUG_CPU_PRINT("%lli " #OPERATOR " %i = %lli\n", *lhs, *rhs, res)\
    }\
    break;\
    case DATATYPE_S32: {\
      int32_t *lhs = (int32_t *) &value1, *rhs = (int32_t *) &value2, res = *lhs + *rhs;\
      result = *(uint64_t *) &res;\
      DEBUG_CPU_PRINT("%i " #OPERATOR " %i = %i\n", *lhs, *rhs, res)\
    }\
    break;\
    case DATATYPE_F: {\
      float *lhs = (float *) &value1, *rhs = (float *) &value2, res = *lhs OPERATOR *rhs;\
      result = *(uint64_t *) &res;\
      DEBUG_CPU_PRINT("%f " #OPERATOR " %f = %f\n", *lhs, *rhs, res)\
    }\
    break;\
    case DATATYPE_D: {\
      double *lhs = (double *) &value1, *rhs = (double *) &value2, res = *lhs OPERATOR *rhs;\
      result = *(uint64_t *) &res;\
      DEBUG_CPU_PRINT("%lf " #OPERATOR " %lf = %lf\n", *lhs, *rhs, res)\
    }\
    break;\
    default:\
      ERR_PRINT("unknown data type indicator: 0x%x\n", datatype)\
      CPU_RAISE_ERROR(ERR_DATATYPE, datatype,)\
  }\
  INJECT\
  REG(reg) = result;\
  update_zero_flag(cpu, reg);

// add <reg> <reg> <value>
static void exec_add(cpu_t *cpu, uint64_t inst) {
  ARITH_OPERATION(+,)
}

// sub <reg> <reg> <value>
static void exec_sub(cpu_t *cpu, uint64_t inst) {
  ARITH_OPERATION(-,)
}

// mul <reg> <reg> <value>
static void exec_mul(cpu_t *cpu, uint64_t inst) {
  ARITH_OPERATION(*,)
}

// div <reg> <reg> <value>
static void exec_div(cpu_t *cpu, uint64_t inst) {
  ARITH_OPERATION(/,)
}

// mod <reg> <value> <value>
static void exec_mod(cpu_t *cpu, uint64_t inst) {
  uint8_t reg;
  uint64_t value1, value2;

  if (!fetch_reg_val_val(cpu, inst, &reg, &value1, &value2, 0, false))
    return;

  int64_t *lhs = (int64_t *) &value1;
  int32_t *rhs = (int32_t *) &value2;
  int64_t result = *lhs % *rhs;

  DEBUG_CPU_PRINT(DEBUG_STR " arithmetic operation: %lli mod %i = %lli\n", *lhs, *rhs, result);
  REG(reg) = result;
  update_zero_flag(cpu, reg);
}

// syscall <value>
static void exec_syscall(cpu_t *cpu, uint64_t inst) {
  // fetch and resolve value, check if OK
  uint64_t value = get_arg_value(cpu, inst, OP_HEADER_SIZE, false);
  if (!CPU_RUNNING) return;

  DEBUG_CPU_PRINT(DEBUG_STR " syscall: invoke operation %llu (", value)

  switch (value) {
    case SYSCALL_PRINT_INT:
#if DEBUG & DEBUG_CPU
      printf("print_int)\n");
#endif
      fprintf(cpu->fp_out, "%llu", REG(REG_GPR));
      break;
    case SYSCALL_PRINT_FLOAT:
#if DEBUG & DEBUG_CPU
      printf("print_float)\n");
#endif
      fprintf(cpu->fp_out, "%f", *(float *) &REG(REG_GPR));
      break;
    case SYSCALL_PRINT_DOUBLE:
#if DEBUG & DEBUG_CPU
      printf("print_double)\n");
#endif
      fprintf(cpu->fp_out, "%lf", *(double *) &REG(REG_GPR));
      break;
    case SYSCALL_PRINT_CHAR:
#if DEBUG & DEBUG_CPU
      printf("print_char)\n");
#endif
      fprintf(cpu->fp_out, "%c", *(char *) &REG(REG_GPR));
      break;
    case SYSCALL_PRINT_STRING: {
#if DEBUG & DEBUG_CPU
      printf("print_string)\n");
#endif
      uint32_t addr = REG(REG_GPR);
      if (!check_memory(addr)) CPU_RAISE_ERROR(ERR_SEGFAULT, addr,)
      fprintf(cpu->fp_out, "%s", (const char *) (cpu->bus.dram.mem + addr));
      break;
    }
    case SYSCALL_READ_INT:
#if DEBUG & DEBUG_CPU
      printf("read_int)\n");
#endif
      fscanf(cpu->fp_in, "%lld", &REG(REG_RET));
      break;
    case SYSCALL_READ_FLOAT:
#if DEBUG & DEBUG_CPU
      printf("read_float)\n");
#endif
      fscanf(cpu->fp_in, "%f", &REG(REG_RET));
      break;
    case SYSCALL_READ_DOUBLE:
#if DEBUG & DEBUG_CPU
      printf("read_double)\n");
#endif
      fscanf(cpu->fp_in, "%lf", &REG(REG_RET));
      break;
    case SYSCALL_READ_CHAR:
#if DEBUG & DEBUG_CPU
      printf("read_char)\n");
#endif
      REG(REG_RET) = fgetc(cpu->fp_in);
      break;
    case SYSCALL_READ_STRING: {
#if DEBUG & DEBUG_CPU
      printf("read_string)\n");
#endif
      uint32_t addr = REG(REG_GPR);
      if (!check_memory(addr)) CPU_RAISE_ERROR(ERR_SEGFAULT, addr,)
      fgets((char *) (cpu->bus.dram.mem + addr), REG(REG_GPR + 1), cpu->fp_in);
      DEBUG_CPU_PRINT(DEBUG_STR " read_string: reading at most %llu bytes from 0x%x... read %lld bytes\n", REG(REG_GPR + 1), addr,
             strlen((char *) (cpu->bus.dram.mem + addr)))
      break;
    }
    case SYSCALL_EXIT:
#if DEBUG & DEBUG_CPU
      printf("exit)\n");
#endif
      CPU_STOP;
      break;
    case SYSCALL_PRINT_REGS:
#if DEBUG & DEBUG_CPU
      printf("debug: print_regs)\n");
#endif
      print_registers(cpu);
      break;
    case SYSCALL_PRINT_MEM: {
#if DEBUG & DEBUG_CPU
      printf("debug: print_mem)\n");
#endif
      uint64_t addr = REG(REG_GPR), size = REG(REG_GPR + 1);
      if (!check_memory(addr)) CPU_RAISE_ERROR(ERR_SEGFAULT, addr,)
      if (!check_memory(addr + size - 1)) CPU_RAISE_ERROR(ERR_SEGFAULT, addr + size - 1,)
      fprintf(cpu->fp_out, "Mem(0x%llx:0x%llx) = { ", addr, addr + size - 1);

      for (uint32_t i = 0; i < size; i++) {
        fprintf(cpu->fp_out, "%02x ", *(cpu->bus.dram.mem + addr + i));
      }

      fprintf(cpu->fp_out, "}");
      break;
    }
    case SYSCALL_PRINT_STACK:
#if DEBUG & DEBUG_CPU
      printf("debug: print_stack)\n");
#endif
      print_stack(cpu);
      break;
    default:
#if DEBUG & DEBUG_CPU
      printf("unknown)\n");
#endif
      ERR_PRINT("invocation of unknown syscall operation (%llu)\n", value)
      CPU_RAISE_ERROR(ERR_SYSCALL, value,)
  }
}

// push <value>
static void exec_push(cpu_t *cpu, uint64_t inst) {
  // fetch and resolve value, check if OK
  uint64_t value = get_arg_value(cpu, inst, OP_HEADER_SIZE, false);
  if (!CPU_RUNNING) return;

  uint32_t data = *(uint32_t *) &value;
  DEBUG_CPU_PRINT(DEBUG_STR " push: value 0x%x to $sp = 0x%llx\n", data, REG(REG_SP))

  PUSH(4, data)
}

// call <addr>
static void exec_call(cpu_t *cpu, uint64_t inst) {
  uint64_t addr = get_arg_addr(cpu, inst, OP_HEADER_SIZE);
  if (!CPU_RUNNING) return;

  DEBUG_CPU_PRINT(DEBUG_STR " call: location 0x%llx with return $ip = 0x%llx\n", addr, REG(REG_IP));

  // push stack frame
  push_stack_frame(cpu);

  // set $ip to location
  REG(REG_IP) = addr;
}

// ret
static void exec_return(cpu_t *cpu, uint64_t inst) {
  // restore stack frame
  pop_stack_frame(cpu);

  DEBUG_CPU_PRINT(DEBUG_STR " ret: return to location 0x%llx\n", REG(REG_IP));
}

// map opcode to handler, which takes CPU and instruction word
typedef void (*exec_t)(cpu_t *, uint64_t);
static exec_t exec_map[OPCODE_MASK + 1] = {NULL};

static void init_exec_map(void) {
  exec_map[OP_LOAD] = exec_load;
  exec_map[OP_LOAD_UPPER] = exec_load_upper;
  exec_map[OP_STORE] = exec_store;
  exec_map[OP_COMPARE] = exec_compare;
  exec_map[OP_NOT] = exec_not;
  exec_map[OP_AND] = exec_and;
  exec_map[OP_OR] = exec_or;
  exec_map[OP_XOR] = exec_xor;
  exec_map[OP_SHL] = exec_shift_left;
  exec_map[OP_SHR] = exec_shift_right;
  exec_map[OP_ADD] = exec_add;
  exec_map[OP_SUB] = exec_sub;
  exec_map[OP_MUL] = exec_mul;
  exec_map[OP_DIV] = exec_div;
  exec_map[OP_MOD] = exec_mod;
  exec_map[OP_PUSH] = exec_push;
  exec_map[OP_CALL] = exec_call;
  exec_map[OP_RET] = exec_return;
  exec_map[OP_SYSCALL] = exec_syscall;
}

void cpu_init(cpu_t *cpu) {
  cpu->fp_out = stdout;
  cpu->fp_in = stdin;
  cpu->addr_interrupt_handler = DEFAULT_INTERRUPT_HANDLER_ADDRESS;

  // clear and configure key registers
  memset(cpu->regs, 0, sizeof(cpu->regs));
  REG(REG_IMR) = 0xffffffffffffffff;
  REG(REG_SP) = DRAM_SIZE;
  REG(REG_FP) = REG(REG_SP);

  // clear memory
  dram_clear(&cpu->bus.dram);

  // initialise execution map?
  if (exec_map[OP_LOAD] == NULL) {
    init_exec_map();
  }
}

uint64_t cpu_fetch(cpu_t *cpu) {
  if (!check_memory(REG(REG_IP))) CPU_RAISE_ERROR(ERR_SEGFAULT, REG(REG_IP), 0)
  return bus_load(&cpu->bus, REG(REG_IP), 64);
}

void cpu_execute(cpu_t *cpu, uint64_t inst) {
  // extract opcode (bits 0-5)
  uint16_t opcode = inst & OPCODE_MASK;

  // switch on opcode prior to conditional test
  switch (opcode) {
    case OP_NOP:
#ifdef HALT_ON_NOP
      CPU_STOP
#endif
      return;
    default: ;
  }

  // check that opcode exists
  const exec_t handler = exec_map[opcode];

  if (handler == NULL) {
    ERR_PRINT("unknown opcode 0x%x (in instruction 0x%llx)\n", opcode, inst)
    CPU_RAISE_ERROR(ERR_OPCODE, opcode,)
  }

  // extract conditional test bits
  uint8_t test_bits = (inst >> OP_CMP_BITS_OFF) & OP_CMP_MASK;

  // test??
  if (test_bits != CMP_NA) {
    // extract cmp bits from both the instruction and the flag register
    uint8_t flag_bits = REG(REG_FLAG) & FLAG_CMP_BITS;

    DEBUG_CPU_PRINT(DEBUG_STR " conditional test: %s (0x%x) ... ", cmp_bit_str(test_bits), test_bits)

    // special case for [N]Z test
    if (test_bits & FLAG_ZERO) {
      bool zero_flag = GET_BIT(REG(REG_FLAG), FLAG_ZERO);

      if ((test_bits == CMP_NZ && zero_flag) || (test_bits == CMP_Z && !zero_flag)) {
#if DEBUG & DEBUG_CPU
        printf(ANSI_RED "fail" ANSI_RESET " ($flag: 0x%x)\n", flag_bits);
#endif
        return;
      }
    }

    // otherwise, compare directly
    else if ((test_bits & FLAG_CMP_BITS) != flag_bits) {
#if DEBUG & DEBUG_CPU
      printf(ANSI_RED "fail" ANSI_RESET " ($flag: 0x%x)\n", flag_bits);
#endif
      return;
    }

#if DEBUG & DEBUG_CPU
    printf(ANSI_GREEN "pass\n");
#endif
  }

  // invoke instruction handle
  handler(cpu, inst);
}

// test if there is an interrupt
static bool is_interrupt(const cpu_t *cpu) {
  // don't allow interrupt stacking
  if (REG(REG_FLAG) & FLAG_IN_INTERRUPT)
    return false;

  return REG(REG_ISR) & REG(REG_IMR);
}

// handle interrupt
static void handle_interrupt(cpu_t *cpu) {
  // save $ip to $iip
  REG(REG_IIP) = REG(REG_IP);

  // disable future interrupts
  REG(REG_FLAG) |= FLAG_IN_INTERRUPT;

  // jump to handler
  REG(REG_IP) = cpu->addr_interrupt_handler;

  DEBUG_CPU_PRINT(DEBUG_STR ANSI_CYAN " interrupt! " ANSI_RESET "$isr=0x%llx, $imr=0x%llx, $iip=0x%llx\n", REG(REG_ISR), REG(REG_IMR), REG(REG_IIP))
}

void cpu_start(cpu_t *cpu) {
  // set running bit, clear error flag
  SET_BIT(REG(REG_FLAG), FLAG_IS_RUNNING);
  REG(REG_FLAG) &= ~(FLAG_ERR_MASK << FLAG_ERR_OFFSET);

#if DEBUG & DEBUG_CPU
  uint32_t counter = 0;
  DEBUG_CPU_PRINT(DEBUG_STR " start cpu; commencing fetch-execute cycle...\n");
#endif

  // fetch-execute cycle until halt
  uint64_t inst;

  while (CPU_RUNNING) {
    // interrupt?
    if (is_interrupt(cpu)) {
      handle_interrupt(cpu);
    }

    DEBUG_CPU_PRINT(DEBUG_STR ANSI_VIOLET " cycle #%i" ANSI_RESET ": ip=0x%llx, inst=", counter++, REG(REG_IP));

    // fetch instruction at instruction pointer
    inst = cpu_fetch(cpu);
    if (CPU_GET_ERROR) break;

#if (DEBUG & DEBUG_CPU) && !(DEBUG & DEBUG_DRAM)
    printf("0x%llx\n", inst);
#endif

    // increment instruction pointer
    REG(REG_IP) += sizeof(inst);

    // execute the instruction
    cpu_execute(cpu, inst);
  }

  DEBUG_CPU_PRINT(DEBUG_STR ANSI_CYAN " halt bit set" ANSI_RESET ": terminating program after cycle %u\n", counter)
}

uint32_t cpu_exit_code(const cpu_t *cpu) {
  uint8_t error = CPU_GET_ERROR;
  return error ? error : REG(REG_RET);
}

static char register_strings[][6] = {
  "$ip",
  "$sp",
  "$fp",
  "$flag",
  "$isr",
  "$imr",
  "$iip",
  "$ret",
  "$k1",
  "$k2"
};

void print_registers(const cpu_t *cpu) {
  uint8_t i;

  // special registers
  for (i = 0; i < REG_GPR; i++) {
    fprintf(cpu->fp_out, "%-8s = 0x%llx\n", register_strings[i], REG(i));
  }

  // GPRs
  for (i = 0; i < REG_PGPR - REG_GPR; i++) {
    fprintf(cpu->fp_out, "$r%02i     = 0x%llx\n", i + 1, REG(REG_GPR + i));
  }

  // PGPRs
  for (i = 0; i < REGISTERS - REG_PGPR; i++) {
    fprintf(cpu->fp_out, "$s%i      = 0x%llx\n", i + 1, REG(REG_PGPR + i));
  }
}

void print_stack(const cpu_t *cpu) {
  uint64_t addr = REG(REG_SP), size = DRAM_SIZE - addr;
  fprintf(cpu->fp_out, "STACK: top = 0x%x -> bottom = 0x%llx = $sp + 1 (%llu bytes)\n", DRAM_SIZE - 1, addr, size);

  uint32_t i = 0, j;
  while (i < size) {
    for (j = 0; j < 8 && i < size; i++, j++) {
      fprintf(cpu->fp_out, "0x%02x ", cpu->bus.dram.mem[addr + i]);
    }

    fprintf(cpu->fp_out, "\n");
  }
}

void print_error(const cpu_t *cpu, bool prefix) {
  uint8_t error = CPU_GET_ERROR;

  if (error == ERR_OK) {
    return;
  }

  if (prefix) {
    fprintf(cpu->fp_out, ERROR_STR " ");
  }

  switch (CPU_GET_ERROR) {
    case ERR_OPCODE:
      fprintf(cpu->fp_out, "E-OPCODE: invalid opcode 0x%llx (at $ip=%llx)\n", REG(REG_RET), REG(REG_IP));
      break;
    case ERR_SEGFAULT:
      fprintf(cpu->fp_out, "E-SEGSEGV: segfault on access of 0x%llx\n", REG(REG_RET));
      break;
    case ERR_REG:
      fprintf(cpu->fp_out, "E-UREG: segfault on access of register at +0x%llx\n", REG(REG_RET));
      break;
    case ERR_SYSCALL:
      fprintf(cpu->fp_out, "E-SYSCALL: system call with unknown opcode 0x%llx\n", REG(REG_RET));
      break;
    case ERR_DATATYPE:
      fprintf(cpu->fp_out, "E-DATATYPE: invalid datatype specifier 0x%llx (at $ip=%llx)\n", REG(REG_RET), REG(REG_IP));
      break;
    default:
      fprintf(cpu->fp_out, "E-UNKNOWN: unknown error, supplied data 0x%llx\n", REG(REG_RET));
  }
}
