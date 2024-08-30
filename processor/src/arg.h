#ifndef _ARG_H_
#define _ARG_H_

#include "cpu.h"

// get argument `<reg>`
uint8_t get_arg_reg(const cpu_t *cpu, uint64_t inst, uint8_t pos);

// extract `<value>` argument from word, fetch value
// cast_imm_double: if true, cast imm to double (as 32-bit imm only, so float)
uint64_t get_arg_value(cpu_t *cpu, uint64_t word, uint8_t pos, bool cast_imm_double);

// extract `<addr>` argument from word (returns address, doesn't extract value)
uint64_t get_arg_addr(cpu_t *cpu, uint64_t word, uint8_t pos);

#endif
