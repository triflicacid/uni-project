#ifndef _ARG_H_
#define _ARG_H_

#define ARG_IMM 0b00
#define ARG_REG 0b01
#define ARG_MEM 0b10
#define ARG_REG_INDIRECT 0b11

#define ARG_REG_SIZE 8
#define ARG_VALUE_SIZE 34
#define ARG_ADDR_SIZE 33

#include "cpu.h"

// get argument `<reg>`
uint8_t get_arg_reg(const cpu_t *cpu, uint64_t inst, uint8_t pos);

// extract `<value>` argument from word, fetch value
// cast_imm_double: if true, cast imm to double (as 32-bit imm only, so float)
uint64_t get_arg_value(cpu_t *cpu, uint64_t word, uint8_t pos, bool cast_imm_double);

// extract `<addr>` argument from word (returns address, doesn't extract value)
uint64_t get_arg_addr(cpu_t *cpu, uint64_t word, uint8_t pos);

#endif
