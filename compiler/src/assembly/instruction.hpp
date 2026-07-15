#pragma once

#include <vector>
#include <ostream>
#include "arg.hpp"
#include "constants.hpp"
#include "line.hpp"

namespace lang::assembly {
  using datatype = constants::inst::datatype::dt;
  using condition = constants::cmp::flag;

  /**
   * @brief Minimal Line subclass: an instruction consisting of a bare mnemonic string with no operands.
   *
   * Common base for every richer instruction variant.
   */
  // base instruction wrapper containing nothing but the mnemonic
  class Instruction : public Line {
    std::string mnemonic_; ///< Base mnemonic text.

  protected:
    std::ostream& _print(std::ostream& os) const override
    { return os << mnemonic_; }

  public:
    /**
     * @brief Constructs a bare-mnemonic instruction.
     * @param mnemonic Instruction mnemonic text.
     */
    explicit Instruction(std::string mnemonic) : mnemonic_(mnemonic) {}
  };

  /**
   * @brief Instruction accepting an arbitrary ordered list of operands, plus optional conditional-test and datatype decorations, built up fluently.
   */
  // a generic instruction which takes any number of arguments, and may have a conditional/datatype flag
  class GenericInstruction : public Instruction {
    std::optional<condition> cond_; ///< Conditional-execution test, if attached via @ref set_conditional.
    std::optional<datatype> datatype_; ///< Datatype qualifier, if attached via @ref set_datatype.
    std::vector<std::unique_ptr<BaseArg>> args_; ///< Operands, in append order.

  protected:
    std::ostream& _print(std::ostream& os) const override;

  public:
    /**
     * @brief Constructs an empty generic instruction with no operands or decorations yet.
     * @param mnemonic Instruction mnemonic text.
     */
    explicit GenericInstruction(std::string mnemonic) : Instruction(std::move(mnemonic)) {}

    /**
     * @brief Attaches a conditional-execution test to this instruction.
     * @param cmp Comparison flag the instruction is conditioned on.
     * @return This instruction, for chaining.
     */
    GenericInstruction& set_conditional(constants::cmp::flag cmp);

    /**
     * @brief Attaches a datatype qualifier to this instruction.
     * @param dt Datatype qualifier.
     * @return This instruction, for chaining.
     */
    GenericInstruction& set_datatype(constants::inst::datatype::dt dt);

    /**
     * @brief Appends one operand to the instruction's argument list.
     * @param arg Operand to append.
     * @return This instruction, for chaining.
     */
    GenericInstruction& add_arg(std::unique_ptr<BaseArg> arg);
  };

  /**
   * @brief Applies set_conditional to an already-constructed, owned instruction, returning it back for chaining.
   * @param i Instruction to modify.
   * @param cmp Comparison flag the instruction is conditioned on.
   * @return The same instruction.
   */
  // shorthand function for setting the conditional test flag of an instruction
  std::unique_ptr<GenericInstruction> set_conditional(std::unique_ptr<GenericInstruction>, constants::cmp::flag cmp);

  /**
   * @brief Applies set_datatype to an already-constructed, owned instruction, returning it back for chaining.
   * @param i Instruction to modify.
   * @param dt Datatype qualifier.
   * @return The same instruction.
   */
  // shorthand function for setting the datatype of an instruction
  std::unique_ptr<GenericInstruction> set_datatype(std::unique_ptr<GenericInstruction>, constants::inst::datatype::dt dt);

  /**
   * @brief Special-cased instruction for the cvt\<from\>2\<to\> family of datatype-conversion opcodes.
   */
  // special instance for `cvt<x>2<y>` instruction
  class ConversionInstruction : public Instruction {
    datatype from_type_; ///< Source datatype.
    datatype to_type_; ///< Destination datatype.
    uint8_t from_reg_; ///< Source register.
    uint8_t to_reg_; ///< Destination register.

  protected:
    std::ostream& _print(std::ostream& os) const override;

  public:
    /**
     * @brief Constructs a fully-specified type-conversion instruction.
     * @param from_type Source datatype.
     * @param from_reg Source register.
     * @param to_type Destination datatype.
     * @param to_reg Destination register.
     */
    ConversionInstruction(datatype from_type, uint8_t from_reg, datatype to_type, uint8_t to_reg)
    : Instruction("cvt"), from_type_(from_type), from_reg_(from_reg), to_type_(to_type), to_reg_(to_reg) {}
  };

  /**
   * @brief Special-cased instruction for the loadi opcode, carrying a full 64-bit immediate wider than an ordinary Arg supports.
   */
  // special instance for the loadi instruction, which accepts a uint64_t immediate argument
  class LoadImmediateInstruction : public Instruction {
    uint8_t reg_; ///< Destination register.
    uint64_t imm_; ///< Immediate value to load.

  protected:
    std::ostream& _print(std::ostream& os) const override;

  public:
    /**
     * @brief Constructs a loadi instruction loading a 64-bit immediate into a target register.
     * @param reg Destination register.
     * @param imm Immediate value to load.
     */
    LoadImmediateInstruction(uint8_t reg, uint64_t imm)
    : Instruction("loadi"), reg_(reg), imm_(imm) {}
  };
}
