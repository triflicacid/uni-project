#pragma once

#include <memory>
#include "basic_block.hpp"
#include "instruction.hpp"

namespace lang::assembly {
  /**
   * @brief Constructs an empty generic instruction with a given mnemonic and no operands.
   * @param mnemonic Instruction mnemonic text.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> instruction(const std::string& mnemonic);

  /**
   * @brief Constructs an "add" instruction: reg_dst = reg + value.
   * @param datatype Datatype the operation is performed as.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Right-hand operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_add(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs an "and" instruction: reg_dst = reg & value.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Right-hand operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_and(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs an unconditional branch instruction.
   * @param to Jump target.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_branch(std::unique_ptr<BaseArg> to);

  /**
   * @brief Constructs a conditional branch instruction.
   * @param guard Comparison flag that must hold for the branch to be taken.
   * @param to Jump target.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_branch(condition guard, std::unique_ptr<BaseArg> to);

  /**
   * @brief Constructs a comparison instruction between a register and another value, using an explicit datatype.
   * @param datatype Datatype the comparison is performed as.
   * @param reg Register to compare.
   * @param value Value to compare against.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_comparison(datatype datatype, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a comparison instruction using the default datatype.
   * @param reg Register to compare.
   * @param value Value to compare against.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_comparison(uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a cvt\<from\>2\<to\> datatype-conversion instruction.
   * @param from_type Source datatype.
   * @param from_reg Source register.
   * @param to_type Destination datatype.
   * @param to_reg Destination register.
   * @return The newly created instruction.
   */
  std::unique_ptr<ConversionInstruction> create_conversion(datatype from_type, uint8_t from_reg, datatype to_type, uint8_t to_reg);

  /**
   * @brief Constructs a "div" instruction: reg_dst = reg / value.
   * @param datatype Datatype the operation is performed as.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Right-hand operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_div(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs an instruction to exit (halt) the process with no exit code.
   * @return The newly created instruction.
   */
  std::unique_ptr<Instruction> create_exit();

  /**
   * @brief Constructs an instruction to exit (halt) the process with a given exit code.
   * @param exit_code Exit code operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_exit(std::unique_ptr<BaseArg> exit_code);

  /**
   * @brief Constructs a signed or zero extension instruction.
   * @param is_signed True for a signed extension, false for a zero extension.
   * @param reg_dst Destination register.
   * @param value Value to extend.
   * @param imm Number of bits to extend by.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_extend(bool is_signed, uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm);

  /**
   * @brief Constructs an interrupt-invocation instruction.
   * @param mask Interrupt flag/mask operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_interrupt(std::unique_ptr<BaseArg> mask);

  /**
   * @brief Constructs a jump-and-link (call) instruction.
   * @param value Call target.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_jump_and_link(std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs an instruction loading a value into a register.
   * @param reg Destination register.
   * @param value Value to load.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_load(uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs an instruction loading a value into the upper half of a register.
   * @param reg Destination register.
   * @param value Value to load.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_load_upper(uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a loadi instruction loading a full 64-bit immediate into a register.
   * @param reg Destination register.
   * @param imm Immediate value to load.
   * @return The newly created instruction.
   */
  std::unique_ptr<LoadImmediateInstruction> create_load_long(uint8_t reg, uint64_t imm);

  /**
   * @brief Appends instructions loading only n bytes of a value into a register, clearing the remainder via a sign/zero extension.
   * @param reg Destination register.
   * @param value Value to load.
   * @param bytes Number of bytes to load (fewer than the register's full width).
   * @param assembly Basic block to append the instructions to.
   * @param is_signed Whether the unused upper bytes should be sign-extended (true) or zero-extended (false).
   */
  // similar to create_load(), but only loads `n` bytes, the rest is cleared in the register
  // important note, *does not* call create_load_long as the type of `value` is not known, so *do not* call if providing a long immediate
  void create_load(uint8_t reg, std::unique_ptr<BaseArg> value, uint8_t bytes, BasicBlock& assembly, bool is_signed);

  /**
   * @brief Constructs a "mod" instruction: reg_dst = reg % value.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Right-hand operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_mod(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a "mul" instruction: reg_dst = reg * value.
   * @param datatype Datatype the operation is performed as.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Right-hand operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_mul(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a no-op instruction.
   * @return The newly created instruction.
   */
  std::unique_ptr<Instruction> create_nop();

  /**
   * @brief Constructs a "not" (bitwise complement) instruction: reg_dst = ~reg.
   * @param reg_dst Destination register.
   * @param reg Operand register.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_not(uint8_t reg_dst, uint8_t reg);

  /**
   * @brief Constructs an "or" instruction: reg_dst = reg | value.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Right-hand operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_or(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs an instruction reserving bytes on the stack ("sub $sp, <bytes>").
   * @param bytes Number of bytes to reserve.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_push(uint8_t bytes);

  /**
   * @brief Constructs an instruction releasing bytes from the stack ("add $sp, <bytes>").
   * @param bytes Number of bytes to release.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_pop(uint8_t bytes);

  /**
   * @brief Constructs a signed-extension instruction.
   * @param reg_dst Destination register.
   * @param value Value to extend.
   * @param imm Number of bits to extend by.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_signed_extend(uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm);

  /**
   * @brief Constructs a left-shift instruction: reg_dst = reg << value.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Shift amount.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_shift_left(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a right-shift instruction: reg_dst = reg >> value.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Shift amount.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_shift_right(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a return instruction with no return value operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_return();

  /**
   * @brief Constructs a return instruction carrying a return value operand.
   * @param value Return value operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_return(std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs an instruction to return from an interrupt handler.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_return_from_interrupt();

  /**
   * @brief Constructs an instruction storing a register's full word at an address.
   * @param reg Register to store.
   * @param address Destination address.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_store(uint8_t reg, std::unique_ptr<BaseArg> address);

  /**
   * @brief Appends instructions storing only n bytes of a register at an address, preserving the remaining bytes at that location.
   * @param reg Register to store.
   * @param address Destination address.
   * @param bytes Number of bytes to store (fewer than the register's full width).
   * @param assembly Basic block to append the instructions to.
   */
  void create_store(uint8_t reg, std::unique_ptr<BaseArg> address, uint8_t bytes, BasicBlock& assembly);

  /**
   * @brief Constructs a "sub" instruction: reg_dst = reg - value.
   * @param datatype Datatype the operation is performed as.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Right-hand operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_sub(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a system-call instruction.
   * @param value Syscall number/identifier operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_system_call(std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs a "xor" instruction: reg_dst = reg ^ value.
   * @param reg_dst Destination register.
   * @param reg Left-hand operand register.
   * @param value Right-hand operand.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_xor(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  /**
   * @brief Constructs an instruction zeroing a register.
   * @param reg Register to zero.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_zero(uint8_t reg);

  /**
   * @brief Constructs a zero-extension instruction.
   * @param reg_dst Destination register.
   * @param value Value to extend.
   * @param imm Number of bits to extend by.
   * @return The newly created instruction.
   */
  std::unique_ptr<GenericInstruction> create_zero_extend(uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm);
}
