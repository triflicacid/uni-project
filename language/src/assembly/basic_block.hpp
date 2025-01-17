#pragma once

#include <deque>
#include "instruction.hpp"

namespace lang::assembly {
  // a basic block represents a sequence of assembly instructions
  // it is labelled and can only contain jump instructions at the end
  class BasicBlock {
    std::optional<std::string> label_;
    std::deque<std::unique_ptr<Instruction>> instructions_;

    BasicBlock() {}
    explicit BasicBlock(std::string label) : label_(std::move(label)) {}

  public:
    BasicBlock(const BasicBlock&) = delete; // important as cannot copy unique_ptr

    const std::optional<std::string>& label() const { return label_; }

    void add(std::unique_ptr<Instruction> i);

    size_t size() const { return instructions_.size(); }

    std::ostream& print(std::ostream& os) const;

    // return a BasicBlock with a unique label
    static std::unique_ptr<BasicBlock> labelled();

    // create a BasicBlock with the given label
    static std::unique_ptr<BasicBlock> labelled(const std::string& label);

    // create an unlabelled BasicBlock
    static std::unique_ptr<BasicBlock> unlabelled();
  };
}
