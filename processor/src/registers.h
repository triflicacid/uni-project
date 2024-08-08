#ifndef _REGISTERS_H_
#define _REGISTERS_H_

#include "util.h"

#define REGISTERS 30
#define REG_IP 0
#define REG_SP 1
#define REG_FP 2
#define REG_FLAG 3
#define REG_RET 4
#define REG_ZERO 5
#define REG_GPR 6
#define REG_PGRP 22

#define FLAG_CMP_BITS (BIT0 | BIT1 | BIT2)
#define CMP_NE 0b000
#define CMP_EQ 0b001
#define CMP_LT 0b010
#define CMP_LE 0b011
#define CMP_Z  0b100
#define CMP_GT 0b110
#define CMP_GE 0b111

#define FLAG_EXEC_STATUS BIT3

// get instruction pointer from the cpu pointer
#define IP(CPU) ((CPU)->regs[REG_IP])

// get the given register from `cpu_t *cpu`
#define REG(OFF) (cpu->regs[OFF])

#endif
