#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include "util.h"

// =============== REGISTERS ===============
#define REGISTERS 32
#define REG_IP   0
#define REG_RIP  1
#define REG_SP   2
#define REG_FP   3
#define REG_FLAG 4
#define REG_ISR  5
#define REG_IMR  6
#define REG_IIP  7
#define REG_RET  8
#define REG_K1   9
#define REG_K2   10
#define REG_GPR  11

// default address of the interrupt handler
#define DEFAULT_INTERRUPT_HANDLER_ADDRESS 0x400

// =============== FLAGS ===============
#define FLAG_ZERO BIT3
#define FLAG_CMP_BITS 0x7
#define CMP_NE 0b0000
#define CMP_EQ 0b0001
#define CMP_LT 0b0010
#define CMP_LE 0b0011
#define CMP_NZ 0b1000
#define CMP_GT 0b0110
#define CMP_GE 0b0111
#define CMP_Z  0b1001
#define CMP_NA 0b1111

#define FLAG_IS_RUNNING BIT4
#define FLAG_IN_INTERRUPT BIT8

// get the given register from `cpu_t *cpu`
#define REG(OFF) (cpu->regs[OFF])

// =============== ERROR FLAGS ===============
#define FLAG_ERR_OFFSET 5
#define FLAG_ERR_MASK 0x7
#define ERR_OK 0b000
#define ERR_OPCODE   0b001
#define ERR_SEGFAULT 0b010
#define ERR_REG      0b011
#define ERR_SYSCALL  0b100
#define ERR_DATATYPE 0b101
#define ERR_UNKNOWN  0b111

// =============== DATA TYPES ===============
#define DATATYPE_SIZE 3
#define DATATYPE_U32 0b000
#define DATATYPE_U64 0b001
#define DATATYPE_S32 0b010
#define DATATYPE_S64 0b011
#define DATATYPE_F   0b100
#define DATATYPE_D   0b101

// =============== SYSTEM CALLS ===============
#define SYSCALL_PRINT_HEX    0
#define SYSCALL_PRINT_INT    1
#define SYSCALL_PRINT_FLOAT  2
#define SYSCALL_PRINT_DOUBLE 3
#define SYSCALL_PRINT_CHAR   4
#define SYSCALL_PRINT_STRING 5

#define SYSCALL_READ_INT     6
#define SYSCALL_READ_FLOAT   7
#define SYSCALL_READ_DOUBLE  8
#define SYSCALL_READ_CHAR    9
#define SYSCALL_READ_STRING  10

#define SYSCALL_EXIT         11

#define SYSCALL_PRINT_REGS   100
#define SYSCALL_PRINT_MEM    101
#define SYSCALL_PRINT_STACK  102

// =============== OPERATIONS ===============
#define OPCODE_MASK 0x3f
#define OPCODE_SIZE 6

#define OP_TEST_BIT BIT6
#define OP_CMP_BITS_OFF 6
#define OP_CMP_MASK 0xf
#define OP_CMP_SIZE 4

#define OP_HEADER_SIZE 10


#define OP_NOP 0x00
#define OP_LOAD 0x01
#define OP_LOAD_UPPER 0x02
#define OP_STORE 0x03
#define OP_COMPARE 0x05
#define OP_CONVERT 0x07
#define OP_NOT 0x10
#define OP_AND 0x11
#define OP_OR  0x12
#define OP_XOR 0x13
#define OP_SHR 0x14
#define OP_SHL 0x15
#define OP_ADD 0x20
#define OP_SUB 0x21
#define OP_MUL 0x22
#define OP_DIV 0x23
#define OP_MOD 0x24
#define OP_JAL 0x29
#define OP_PUSH 0x30
#define OP_SYSCALL 0x3f

// if defined, behaviour is to halt on NOP
#define HALT_ON_NOP

#endif
