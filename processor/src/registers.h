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

#define FLAG_CMP_EQ BIT0
#define FLAG_CMP_MAG BIT1
#define FLAG_ZERO BIT2
#define FLAG_EXEC_STATUS BIT3

// get instruction pointer from the cpu pointer
#define IP(CPU) ((CPU)->regs[REG_IP])

// get the given register from `cpu_t *cpu`
#define REG(OFF) (cpu->regs[OFF])

#endif
