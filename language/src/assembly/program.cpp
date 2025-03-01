#include "program.hpp"

lang::assembly::Program::Program(std::string start_label) : current_(0) {
  insert_at(0, BasicBlock::labelled(start_label));
}

void lang::assembly::Program::insert_at(int index, std::unique_ptr<BasicBlock> block) {
  current_ = index;
  labels_.insert({block->label(), *block});
  blocks_.insert(blocks_.begin() + index, std::move(block));
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
  for (int i = 0; i < blocks_.size(); i++) {
    if (blocks_[i]->label() == label) {
      current_ = i;
      return true;
    }
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
    block->print(os);
    if (block != blocks_.back()) os << std::endl;
  }
  return os;
}

lang::assembly::BasicBlock& lang::assembly::Program::get(const std::string& label) {
  return labels_.find(label)->second;
}

void lang::assembly::Program::add_location(Location loc) {
  locations_.push(std::move(loc));
}

optional_ref<const Location> lang::assembly::Program::location() const {
  if (locations_.empty()) return {};
  return std::ref(locations_.top());
}

void lang::assembly::Program::remove_location() {
  if (!locations_.empty()) locations_.pop();
}

void lang::assembly::Program::set_location(Location loc) {
  if (locations_.empty()) {
    locations_.push(std::move(loc));
  } else {
    locations_.top() = std::move(loc);
  }
}

void lang::assembly::Program::update_line_origins(int start, bool sudo) const {
  auto origin = location();
  if (!origin) return;
  update_line_origins(origin->get(), start, sudo);
}

void lang::assembly::Program::update_line_origins(const Location& origin, int start, bool sudo) const {
  // get the current block
  BasicBlock& block = current();

  // set start to end if needed
  if (start == -1) start = block.size() - 1;

  for (int i = start; i < block.size(); i++) {
    if (sudo || !block[i].origin())
      block[i].origin(origin);
  }
}
