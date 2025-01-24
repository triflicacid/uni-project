#pragma once

#include <memory>
#include "basic_block.hpp"
#include "instruction.hpp"

namespace lang::assembly {
  std::unique_ptr<GenericInstruction> instruction(const std::string& mnemonic);

  std::unique_ptr<GenericInstruction> create_add(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  std::unique_ptr<GenericInstruction> create_and(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  // creates an unconditional branch
  std::unique_ptr<GenericInstruction> create_branch(std::unique_ptr<BaseArg> to);

  // creates a conditional branch
  std::unique_ptr<GenericInstruction> create_branch(condition guard, std::unique_ptr<BaseArg> to);

  std::unique_ptr<ConversionInstruction> create_conversion(datatype from_type, uint8_t from_reg, datatype to_type, uint8_t to_reg);

  std::unique_ptr<GenericInstruction> create_div(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  // instruction to exit (halt) the process
  std::unique_ptr<Instruction> create_exit();

  // instruction to exit (halt) the process with the given code
  std::unique_ptr<GenericInstruction> create_exit(std::unique_ptr<BaseArg> exit_code);

  // create a signed or zero extension
  std::unique_ptr<GenericInstruction> create_extend(bool is_signed, uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm);

  // create an interrupt invocation with the given flag
  std::unique_ptr<GenericInstruction> create_interrupt(std::unique_ptr<BaseArg> mask);

  std::unique_ptr<GenericInstruction> create_jump_and_link(std::unique_ptr<BaseArg> value);

  // load value into the given register
  std::unique_ptr<GenericInstruction> create_load(uint8_t reg, std::unique_ptr<BaseArg> value);

  // load value into upper-half of the given register
  std::unique_ptr<GenericInstruction> create_load_upper(uint8_t reg, std::unique_ptr<BaseArg> value);

  // load the given immediate value into the register (used for large immediates wider than 4 bytes)
  std::unique_ptr<LoadImmediateInstruction> create_load_long(uint8_t reg, uint64_t imm);

  // similar to create_load(), but only loads `n` bytes, the rest is cleared in the register
  // important note, *does not* call create_load_long as the type of `value` is not known, so *do not* call if providing a long immediate
  void create_load(uint8_t reg, std::unique_ptr<BaseArg> value, uint8_t bytes, BasicBlock& assembly, bool is_signed);

  std::unique_ptr<GenericInstruction> create_mod(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  std::unique_ptr<GenericInstruction> create_mul(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  std::unique_ptr<Instruction> create_nop();

  std::unique_ptr<GenericInstruction> create_not(uint8_t reg_dst, uint8_t reg);

  std::unique_ptr<GenericInstruction> create_or(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  // push certain bytes to the stack i.e., "sub $sp, <bytes>"
  std::unique_ptr<GenericInstruction> create_push(uint8_t bytes);

  // pop certain bytes from the stack i.e., "add, <bytes>"
  std::unique_ptr<GenericInstruction> create_pop(uint8_t bytes);

  std::unique_ptr<GenericInstruction> create_signed_extend(uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm);

  std::unique_ptr<GenericInstruction> create_shift_left(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  std::unique_ptr<GenericInstruction> create_shift_right(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  std::unique_ptr<GenericInstruction> create_return();

  std::unique_ptr<GenericInstruction> create_return(std::unique_ptr<BaseArg> value);

  // create an instruction to return from an interrupt
  std::unique_ptr<GenericInstruction> create_return_from_interrupt();

  // store register at the given memory address (word) / register indirect address
  std::unique_ptr<GenericInstruction> create_store(uint8_t reg, std::unique_ptr<BaseArg> address);

  // similar to create_store(), but only stores `n` bytes, preserves memory except `n` bytes
  void create_store(uint8_t reg, std::unique_ptr<BaseArg> address, uint8_t bytes, BasicBlock& assembly);

  std::unique_ptr<GenericInstruction> create_sub(datatype datatype, uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  std::unique_ptr<GenericInstruction> create_system_call(std::unique_ptr<BaseArg> value);

  std::unique_ptr<GenericInstruction> create_xor(uint8_t reg_dst, uint8_t reg, std::unique_ptr<BaseArg> value);

  std::unique_ptr<GenericInstruction> create_zero(uint8_t reg);

  std::unique_ptr<GenericInstruction> create_zero_extend(uint8_t reg_dst, std::unique_ptr<BaseArg> value, uint32_t imm);
}
