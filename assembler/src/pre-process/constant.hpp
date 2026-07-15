#pragma once

#include <string>
#include "location.hpp"

namespace assembler::pre_processor {
  /** @brief A `%define`d constant: the source location it was defined at, and its substitution text. */
  struct Constant {
    Location loc; ///< Source location the constant was defined at.
    std::string value; ///< Substitution text.
  };
}
