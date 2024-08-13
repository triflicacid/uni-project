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

// CPU data structure
typedef struct cpu {
  uint64_t regs[REGISTERS];  // register store
  bus_t bus;  // connected to bus to access memory
  FILE *fp_out;
  FILE *fp_in;
} cpu_t;

void cpu_init(cpu_t *cpu);

// fetch next instruction, DO NOT increment ip
uint64_t cpu_fetch(const cpu_t *cpu);

// execute the given instruction
void cpu_execute(cpu_t *cpu, uint64_t inst);

// commence fetch-execute cycle until halt or error
// clears bit flag
void cpu_start(cpu_t *cpu);

// print contents of all registers as hexadecimal
void print_registers(cpu_t *cpu);

#endif
