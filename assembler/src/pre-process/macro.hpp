#pragma once

#include <string>
#include <utility>
#include <vector>

namespace assembler::pre_processor {
  /** @brief A macro definition: its declared parameter names and body lines, to be expanded at each call site. */
  struct Macro {
    Location loc;
    std::vector<std::string> params;
    std::vector<std::string> lines; // Lines in macro's body

    /**
     * @brief Construct a macro with an empty body.
     * @param loc Source location the macro was defined at.
     * @param params Names of the macro's parameters.
     */
    Macro(Location loc, std::vector<std::string> params) : loc(std::move(loc)), params(std::move(params)) {}
  };
}
