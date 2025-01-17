#include "basic_block.hpp"

void lang::assembly::BasicBlock::add(std::unique_ptr<Instruction> i) {
  instructions_.push_back(std::move(i));
}

std::ostream& lang::assembly::BasicBlock::print(std::ostream& os) const {
  // print label if provided
  if (label_.has_value()) os << label_.value() << ":" << std::endl;

  // print our instructions
  for (auto& instruction : instructions_) {
    instruction->print(os) << std::endl;
  }

  return os;
}

std::unique_ptr<lang::assembly::BasicBlock> lang::assembly::BasicBlock::labelled() {
  static int _current_id = 0;
  return std::unique_ptr<BasicBlock>(new BasicBlock("block" + std::to_string(_current_id++)));
}

std::unique_ptr<lang::assembly::BasicBlock> lang::assembly::BasicBlock::labelled(const std::string& label) {
  return std::unique_ptr<BasicBlock>(new BasicBlock(label));
}

std::unique_ptr<lang::assembly::BasicBlock> lang::assembly::BasicBlock::unlabelled() {
  return std::unique_ptr<BasicBlock>(new BasicBlock);
}
