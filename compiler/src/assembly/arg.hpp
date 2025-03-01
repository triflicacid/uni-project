#pragma once

#include "constants.hpp"
#include <ostream>
#include <memory>

namespace lang::assembly {
  struct BaseArg {
    virtual ~BaseArg() = default;

    virtual std::ostream& print(std::ostream& os) const = 0;
  };

  // an argument which references a label
  class LabelArg : public BaseArg {
    const std::string label_;
    int offset_;

  public:
    LabelArg(const std::string& label, int offset) : label_(label), offset_(offset) {}

    std::ostream& print(std::ostream &os) const override;
  };

  class BasicBlock;

  // a special form of `LabelArg` which references a BasicBlock
  // error is BasicBlock does not have a label
  class BlockReferenceArg : public BaseArg {
    const BasicBlock& block_;
    int offset_;

  public:
    BlockReferenceArg(const BasicBlock& block, int offset) : block_(block), offset_(offset) {}

    std::ostream& print(std::ostream &os) const override;
  };

  // a generic assembly argument: imm, mem, reg, reg_indirect
  class Arg : public BaseArg {
    constants::inst::arg type_;
    uint32_t value_;

  public:
    Arg(constants::inst::arg type, uint32_t value) : type_(type), value_(value) {}

    std::ostream& print(std::ostream &os) const override;

    // create an immediate argument
    static std::unique_ptr<Arg> imm(uint32_t x);

    // create a register argument
    static std::unique_ptr<Arg> reg(uint8_t reg);

    // create a memory address argument
    static std::unique_ptr<Arg> mem(uint32_t addr);

    // create a register-indirect argument
    static std::unique_ptr<Arg> reg_indirect(uint8_t reg, int32_t offset = 0);

    // create an argument to a label
    static std::unique_ptr<LabelArg> label(const std::string& label, int offset = 0);

    // create an argument referencing a BasicBlock
    static std::unique_ptr<BlockReferenceArg> label(const assembly::BasicBlock& block, int offset = 0);
  };
}
