#pragma once

namespace lang::memory {
  // describe the physical storage location of a symbol
  struct StorageLocation {
    enum Type {
      Block,
      Stack
    };

    Type type;
    union {
      uint64_t offset;
      std::reference_wrapper<assembly::BasicBlock> block;
    };

    StorageLocation(uint64_t offset) : type(Stack), offset(offset) {}
    StorageLocation(assembly::BasicBlock& block) : type(Block), block(block) {}
  };
}
