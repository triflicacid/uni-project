#pragma once

namespace lang::assembly {
  class BasicBlock;
}

namespace lang::memory {
  // describe the physical storage location of a symbol
  struct StorageLocation {
    enum Type {
      Block,
      Stack, // offset `$sp + n`
    };

    Type type;
    union {
      uint64_t offset;
      std::reference_wrapper<assembly::BasicBlock> block;
    };

    // location is based globally, tied to a block
    static StorageLocation global(assembly::BasicBlock& block) { return StorageLocation{ .type = Block, .block = block }; }

    // location is `offset` on the stack
    static StorageLocation stack(uint64_t offset) { return StorageLocation{ .type = Stack, .offset = offset }; }
  };
}
