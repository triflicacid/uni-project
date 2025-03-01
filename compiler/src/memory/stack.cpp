#include "stack.hpp"
#include "assembly/create.hpp"

void lang::memory::StackManager::push(uint8_t bytes) {
  if (bytes == 0) return;
  offset_ += bytes;
  program_.current().add(assembly::create_push(bytes));
}

void lang::memory::StackManager::pop(uint8_t bytes) {
  if (bytes == 0) return;
  offset_ -= bytes;
  program_.current().add(assembly::create_pop(bytes));
}

void lang::memory::StackManager::push_frame(bool generate_code) {
  frames_.push_front(offset_);
  offset_ = 0;
  if (!generate_code) return;

  // save $sp into $fp
  program_.current().add(assembly::create_load(
      constants::registers::fp,
      assembly::Arg::reg(constants::registers::sp)
    ));
  program_.current().back().comment() << "push frame";
}

void lang::memory::StackManager::pop_frame(bool generate_code) {
  if (frames_.empty()) return;
  offset_ = frames_.front();
  frames_.pop_front();

  if (!generate_code) return;

  // restore $fp into $sp
  program_.current().add(assembly::create_load(
      constants::registers::sp,
      assembly::Arg::reg(constants::registers::fp)
  ));
  program_.current().back().comment() << "restore frame";
}

uint64_t lang::memory::StackManager::peek_frame(unsigned int n) const {
  return frames_.at(n);
}
