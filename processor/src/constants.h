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
#define FLAG_ZERO BIT3
#define FLAG_EQ BIT0
#define FLAG_CMP_BITS (BIT0 | BIT1 | BIT2)
#define CMP_NE 0b0000
#define CMP_EQ 0b0001
#define CMP_LT 0b0010
#define CMP_LE 0b0011
#define CMP_NZ 0b0100
#define CMP_GT 0b0110
#define CMP_GE 0b0111
#define CMP_Z  0b1000

#define FLAG_IS_RUNNING BIT4
#define FLAG_IS_ERROR BIT5

// get instruction pointer from the cpu pointer
#define IP(CPU) ((CPU)->regs[REG_IP])

// get the given register from `cpu_t *cpu`
#define REG(OFF) (cpu->regs[OFF])

// =============== OPERATIONS ===============
#define OPCODE_MASK 0x3f
#define OPCODE_SIZE 6

#define OP_TEST_BIT BIT6
#define OP_CMP_BITS_OFF 7

#define ARG_IMM 0b00
#define ARG_MEM 0b01
#define ARG_REG 0b10
#define ARG_REG_INDIRECT 0b11

#define ARG_REG_SIZE 8
#define ARG_VALUE_SIZE 34
#define ARG_ADDR_SIZE 34
#define OP_HEADER_SIZE 10

#define DATATYPE_SIZE 3
#define DATATYPE_U32 0b000
#define DATATYPE_U64 0b001
#define DATATYPE_S32 0b010
#define DATATYPE_S64 0b011
#define DATATYPE_F   0b100
#define DATATYPE_D   0b101


#define OP_NOP 0x00
#define OP_LOAD 0x01
#define OP_LOAD_UPPER 0x02
#define OP_STORE 0x03

#define OP_COMPARE 0x10

// if defined, behaviour is to halt on NOP
#define HALT_ON_NOP

#endif
