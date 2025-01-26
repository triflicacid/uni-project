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

void lang::memory::StackManager::push_frame() {
  frames_.push_front(offset_);
  // TODO assembly code
}

void lang::memory::StackManager::pop_frame() {
  if (frames_.empty()) return;
  offset_ = frames_.front();
  frames_.pop_front();
  // TODO assembly code
}

uint64_t lang::memory::StackManager::peek_frame(unsigned int n) const {
  return frames_.at(n);
}
