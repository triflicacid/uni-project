#include "storage_location.hpp"
#include "assembly/basic_block.hpp"

std::unique_ptr <lang::assembly::BaseArg> lang::memory::StorageLocation::resolve() const {
  switch (type) {
    case memory::StorageLocation::Block:
      return assembly::Arg::label(block, offset);
    case memory::StorageLocation::Stack:
      return assembly::Arg::reg_indirect(constants::registers::fp, -base_offset - offset); // remember to negate location from $fp as stack grows downwards
  }
}

lang::memory::StorageLocation lang::memory::StorageLocation::global(lang::assembly::BasicBlock& block, int offset) {
  return StorageLocation{ .type = Block, .block = block, .offset = offset };
}

lang::memory::StorageLocation lang::memory::StorageLocation::stack(uint64_t stack_offset, int offset) {
  return StorageLocation{ .type = Stack, .base_offset = stack_offset, .offset = offset };
}

bool lang::memory::StorageLocation::operator==(const lang::memory::StorageLocation& other) const {
  if (type != other.type || offset != other.offset) return false;
  if (type == Stack) return base_offset == other.base_offset;
  return &block.get() == &other.block.get();
}

lang::memory::StorageLocation lang::memory::StorageLocation::operator+(int offset) const {
  offset += this->offset;

  if (type == Stack) {
    return stack(base_offset, offset);
  } else {
    return global(block, offset);
  }
}

lang::memory::StorageLocation lang::memory::StorageLocation::operator-(int offset) const {
  return *this + -offset;
}
