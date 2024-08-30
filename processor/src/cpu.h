#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "bus.h"
#include "constants.h"

// write 64-bit word to word offset of `cpu_t *cpu`
#define MEMWRITE(OFFSET, VALUE) dram_store(&cpu->bus.dram, 8 * (OFFSET), 64, VALUE)

// get 64-bit word to word offset of `cpu_t *cpu`
#define MEMREAD(OFFSET) dram_load(&cpu->bus.dram, 8 * (OFFSET), 64)

// check if CPU is running
#define CPU_RUNNING GET_BIT(REG(REG_FLAG), FLAG_IS_RUNNING)

// stop CPU
#define CPU_STOP CLEAR_BIT(REG(REG_FLAG), FLAG_IS_RUNNING);

// get error flag
#define CPU_GET_ERROR ((REG(REG_FLAG) >> FLAG_ERR_OFFSET) & FLAG_ERR_MASK)

// halt CPU, raise error with CODE, write VAL to $ret, and return RET
#define CPU_RAISE_ERROR(CODE, VAL, RET) { \
  CLEAR_BIT(REG(REG_FLAG), FLAG_IS_RUNNING); \
  REG(REG_FLAG) |= (CODE & FLAG_ERR_MASK) << FLAG_ERR_OFFSET; \
  REG(REG_RET) = VAL; \
  return RET; \
}

// CPU data structure
typedef struct cpu {
  uint64_t regs[REGISTERS];  // register store
  bus_t bus;  // connected to bus to access memory
  FILE *fp_out;
  FILE *fp_in;
} cpu_t;

void cpu_init(cpu_t *cpu);

// fetch next instruction, DO NOT increment ip
uint64_t cpu_fetch(cpu_t *cpu);

// execute the given instruction
void cpu_execute(cpu_t *cpu, uint64_t inst);

// commence fetch-execute cycle until halt or error
// clears bit flag
void cpu_start(cpu_t *cpu);

// get the CPUs exit code
uint32_t cpu_exit_code(const cpu_t *cpu);

// print contents of all registers as hexadecimal
void print_registers(const cpu_t *cpu);

// print error details (doesn't print if no error)
void print_error(const cpu_t *cpu, bool prefix);

// print contents of stack as hexadecimal bytes
void print_stack(const cpu_t *cpu);

// check if the given memory address is valid
bool check_memory(uint32_t address);

// check if the given register offset is valid
bool check_register(uint8_t reg);

#endif
