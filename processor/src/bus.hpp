#pragma once

#include "dram.hpp"

namespace processor {
  // a bus is a transfer route to a DRAM block
  // create abstraction layer to allow for middleware
  struct bus {
    dram mem;

    [[nodiscard]] uint64_t load(uint64_t addr, uint8_t size) const;

    void store(uint64_t addr, uint8_t size, uint64_t bytes);
  };
}
