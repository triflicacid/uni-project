#include "arg.h"
#include "debug.h"

// extract `<reg>` argument from data
static uint8_t arg_reg(const cpu_t *cpu, uint32_t data) {
  return data;
}

uint8_t get_arg_reg(const cpu_t *cpu, uint64_t inst, uint8_t pos) {
  return inst >> pos;
}

// extract `<addr>` argument from data
static uint32_t arg_addr(cpu_t *cpu, uint32_t data) {;
  if (!check_memory(data)) CPU_RAISE_ERROR(ERR_SEGFAULT, data, 0)
  return data;
}

// extract `<register indirect>` address argument from word, return offset
static uint32_t arg_reg_indirect(cpu_t *cpu, uint32_t data) {
  uint8_t reg = data & 0xff;
  if (!check_register(reg)) CPU_RAISE_ERROR(ERR_REG, reg, 0)

  int32_t offset = (data >> 8) & 0xffffff;
  data = REG(reg) + offset;
  if (!check_memory(data)) CPU_RAISE_ERROR(ERR_SEGFAULT, data, 0)

  return data;
}

uint64_t get_arg_value(cpu_t *cpu, uint64_t word, uint8_t pos, bool cast_imm_double) {
  // switch on indicator bits, extract data after
  uint8_t indicator = (word >> pos) & 0x3;
  uint64_t data = word >> (pos + 2);

  switch (indicator) {
    case ARG_IMM:
      if (cast_imm_double) {
        double d = *(float *) &data;
        return *(uint64_t *) &d;
      }

      return data;
    case ARG_MEM:
      return bus_load(&cpu->bus, arg_addr(cpu, data), 64);
    case ARG_REG:
      return arg_reg(cpu, data);
    case ARG_REG_INDIRECT:
      return bus_load(&cpu->bus, arg_reg_indirect(cpu, data), 64);
    default:
      ERR_PRINT("resolve <value>: unknown indicator bit pattern 0x%x\n", indicator)
      CPU_RAISE_ERROR(ERR_UNKNOWN, indicator, 0)
  }
}

// extract `<addr>` argument from word (returns address, doesn't extract value)
uint64_t get_arg_addr(cpu_t *cpu, uint64_t word, uint8_t pos) {
  // switch on indicator bits, extract data after
  uint8_t indicator = (word >> pos) & 0x1;
  uint64_t data = word >> (pos + 1);

  switch (0x2 | indicator) {
    case ARG_MEM:
      return arg_addr(cpu, data);
    case ARG_REG_INDIRECT:
      return arg_reg_indirect(cpu, data);
    default:
      ERR_PRINT("resolve <addr>: unknown indicator bit pattern 0x%x\n", indicator)
      CPU_RAISE_ERROR(ERR_UNKNOWN, indicator, 0)
  }
}
