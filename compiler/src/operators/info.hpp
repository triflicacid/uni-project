#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

namespace lang::ops {
  // describes an operator
  struct OperatorInfo {
    uint8_t precedence; // operator precedence, 1 is loosest (higher = stronger)
    bool right_associative;
    bool overloadable = true;
  };

  // map operator to operator info for builtin binary operators
  extern std::unordered_map<std::string, const OperatorInfo> builtin_binary;

  // map operator to operator info for builtin unary operators
  extern std::unordered_map<std::string, const OperatorInfo> builtin_unary;

  // operator infor for a generic binary operator (not built in)
  extern const OperatorInfo generic_binary;

  // operator info for a generic unary operator (not built in)
  extern const OperatorInfo generic_unary;
}
