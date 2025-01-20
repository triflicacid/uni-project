#pragma once

#include <vector>
#include <ostream>
#include "arg.hpp"
#include "constants.hpp"
#include "line.hpp"

namespace lang::assembly {
  using datatype = constants::inst::datatype::dt;
  using condition = constants::cmp::flag;

  // base instruction wrapper containing nothing but the mnemonic
  class Instruction : public Line {
    std::string mnemonic_; // base mnemonic

  protected:
    std::ostream& _print(std::ostream& os) const override
    { return os << mnemonic_; }

  public:
    explicit Instruction(std::string mnemonic) : mnemonic_(mnemonic) {}
  };

  // a generic instruction which takes any number of arguments, and may have a conditional/datatype flag
  class GenericInstruction : public Instruction {
    std::optional<condition> cond_;
    std::optional<datatype> datatype_;
    std::vector<std::unique_ptr<BaseArg>> args_;

  protected:
    std::ostream& _print(std::ostream& os) const override;

  public:
    explicit GenericInstruction(std::string mnemonic) : Instruction(std::move(mnemonic)) {}

    GenericInstruction& set_conditional(constants::cmp::flag cmp);

    GenericInstruction& set_datatype(constants::inst::datatype::dt dt);

    GenericInstruction& add_arg(std::unique_ptr<BaseArg> arg);
  };

  // special instance for `cvt<x>2<y>` instruction
  class ConversionInstruction : public Instruction {
    datatype from_type_, to_type_;
    uint8_t from_reg_, to_reg_;

  protected:
    std::ostream& _print(std::ostream& os) const override;

  public:
    ConversionInstruction(datatype from_type, uint8_t from_reg, datatype to_type, uint8_t to_reg)
    : Instruction("cvt"), from_type_(from_type), from_reg_(from_reg), to_type_(to_type), to_reg_(to_reg) {}
  };

  // special instance for the loadi instruction, which accepts a uint64_t immediate argument
  class LoadImmediateInstruction : public Instruction {
    uint8_t reg_;
    uint64_t imm_;

  protected:
    std::ostream& _print(std::ostream& os) const override;

  public:
    LoadImmediateInstruction(uint8_t reg, uint64_t imm)
    : Instruction("loadi"), reg_(reg), imm_(imm) {}
  };
}
