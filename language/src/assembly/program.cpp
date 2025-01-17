#include "program.hpp"

lang::assembly::Program::Program(std::string start_label) : current_(0) {
  insert_at(0, BasicBlock::labelled(start_label));
}

void lang::assembly::Program::insert_at(int index, std::unique_ptr<BasicBlock> block) {
  current_ = index;
  blocks_.insert(blocks_.begin() + index, std::move(block));

  // add to label map if necessary
  if (auto& b = blocks_[index]; b->label().has_value()) {
    labels_.insert({b->label().value(), index});
  }
}

bool lang::assembly::Program::select(const lang::assembly::BasicBlock& block) {
  for (int i = 0; i < blocks_.size(); i++) {
    if (blocks_[i].get() == &block) {
      current_ = i;
      return true;
    }
  }

  return false;
}

bool lang::assembly::Program::select(const std::string& label) {
  if (auto it = labels_.find(label); it != labels_.end()) {
    current_ = it->second;
    return true;
  }

  return false;
}

void lang::assembly::Program::insert(Position pos, std::unique_ptr<BasicBlock> block) {
  switch (pos) {
    case Position::Start:
      insert_at(0, std::move(block));
      break;
    case Position::Before:
      insert_at(current_, std::move(block));
      break;
    case Position::After:
      insert_at(current_ + 1, std::move(block));
      break;
    case Position::End:
      insert_at(blocks_.size(), std::move(block));
      break;
  }
}

void lang::assembly::Program::select(lang::assembly::Position pos) {
  switch (pos) {
    case Position::Start:
      current_ = 0;
      break;
    case Position::Previous:
      if (current_ > 0) current_--;
      break;
    case Position::Next:
      if (current_ < blocks_.size() - 1) current_++;
      break;
    case Position::End:
      current_ = blocks_.size() - 1;
      break;
  }
}

std::ostream& lang::assembly::Program::print(std::ostream& os) const {
  for (auto& block : blocks_) {
    block->print(os) << std::endl;
  }
  return os;
}
