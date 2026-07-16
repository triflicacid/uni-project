#pragma once

#include <string>

namespace lang::control_flow {
  /**
   * @brief Records the entry and exit block labels of an enclosing loop, used to resolve break/continue targets.
   */
  struct LoopContext {
    std::string start; ///< Label of the block at the start of the loop, targeted by `continue`.
    std::string end; ///< Label of the block at the end of the loop, targeted by `break`.
  };
}
