#pragma once

#include "assembly/arg.hpp"

namespace lang::assembly {
  class BasicBlock;
}

namespace lang::memory {
  // describe the physical storage location of a symbol
  struct StorageLocation {
    enum Type {
      Block,
      Stack, // offset `-n($fp)`
    };

    Type type;
    union {
      int base_offset;
      std::reference_wrapper<assembly::BasicBlock> block;
    };
    int offset; // offset from base

    std::unique_ptr<assembly::BaseArg> resolve() const;

    bool operator==(const StorageLocation& other) const;

    // add an offset to this block
    StorageLocation operator+(int offset) const;
    StorageLocation operator-(int offset) const;

    // location is based globally, tied to a block
    static StorageLocation global(assembly::BasicBlock& block, int offset = 0);

    // location is `offset` on the stack
    static StorageLocation stack(int stack_offset, int offset = 0);
  };
}
