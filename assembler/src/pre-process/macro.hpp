#pragma once

#include <string>
#include <utility>
#include <vector>

namespace assembler::pre_processor {
  struct Macro {
    Location loc;
    std::vector<std::string> params;
    std::vector<std::string> lines; // Lines in macro's body

    Macro(Location loc, std::vector<std::string> params) : loc(std::move(loc)), params(std::move(params)) {}
  };
}
