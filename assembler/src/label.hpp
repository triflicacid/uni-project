#pragma once

namespace assembler {
  /** @brief A label: the source location it was declared at, and the address it resolves to once assembled. */
  struct Label {
    Location loc; ///< Source location the label was declared at.
    uint64_t addr = 0; ///< Address the label resolves to once assembled.
  };
}
