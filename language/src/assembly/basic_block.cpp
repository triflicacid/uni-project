#include "basic_block.hpp"
#include "config.hpp"

lang::assembly::BasicBlock& lang::assembly::BasicBlock::add(std::unique_ptr<Line> i) {
  contents_.push_back(std::move(i));
  return *this;
}

std::ostream& lang::assembly::BasicBlock::print(std::ostream& os) const {
  // print label and comment
  os << label_ << ":";
  if (std::string str = comment_.str(); !str.empty()) os << "  ; " << str;
  os << std::endl;

  // print our instructions
  for (auto& line : contents_) {
    if (conf::indent_asm_code) os << "  ";
    line->print(os) << std::endl;
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
