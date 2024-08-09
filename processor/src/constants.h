#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_

#include "util.h"

// =============== REGISTERS ===============

#define REGISTERS 30
#define REG_IP 0
#define REG_SP 1
#define REG_FP 2
#define REG_FLAG 3
#define REG_RET 4
#define REG_ZERO 5
#define REG_GPR 6
#define REG_PGPR 22

// =============== FLAGS ===============
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

// =============== OPERATIONS ===============
#define OP_TEST_BIT BIT6
#define OP_CMP_BITS 0x380
#define OP_CMP_BITS_OFF 7

#define ARG_IMM 0b00
#define ARG_MEM 0b01
#define ARG_REG 0b10
#define ARG_REG_OFF 0b11

#define ARG_REG_SIZE 8
#define ARG_VALUE_SIZE 34
#define ARG_ADDR_SIZE 34
#define OP_HEADER_SIZE 10

#define OP_NOP 0x00
#define OP_LOAD 0x01
#define OP_LOAD_UPPER 0x02
#define OP_STORE 0x03

// if defined, behaviour is to halt on NOP
#define HALT_ON_NOP

#endif
