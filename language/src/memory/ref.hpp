#pragma once

#include <cstdint>

namespace lang::memory {
  // describe reference to an item, either a register or a memory address
  struct Ref {
    enum Type {
      Register,
      Memory
    };

    Type type;
    uint64_t offset;

    bool operator==(const Ref& other) const
    { return type == other.type && offset == other.offset; }
  };
}
