#ifndef _CPU_H_
#define _CPU_H_

#include <stdint.h>
#include <stdio.h>

#include "bus.h"

#define REGISTERS 30
#define REG_IP 0
#define REG_SP 1
#define REG_FP 2
#define REG_FLAG 3
#define REG_RET 4
#define REG_ZERO 5
#define REG_GPR 6
#define REG_PGRP 22

#define DEBUG_CPU  0x1
#define DEBUG_DRAM 0x2
#ifndef DEBUG
#define DEBUG 0x00
#endif

// CPU data structure
typedef struct cpu {
  uint64_t regs[REGISTERS];  // register store
  bus_t bus;  // connected to bus to access memory
  FILE *fp_out;
  FILE *fp_in;
} cpu_t;

void cpu_init(cpu_t *cpu);

#endif
