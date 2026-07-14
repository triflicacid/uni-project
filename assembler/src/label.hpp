#pragma once

namespace assembler {
  /** @brief A label: the source location it was declared at, and the address it resolves to once assembled. */
  struct Label {
    Location loc;
    uint64_t addr = 0;
  };
}
