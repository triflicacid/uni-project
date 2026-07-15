#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>

namespace lang::ops {
  /**
   * @brief Describes an operator's parse-time syntax: precedence, associativity, and whether it may be user-overloaded.
   */
  // describes an operator
  struct OperatorInfo {
    uint8_t precedence; ///< Operator precedence; 1 is loosest (higher binds stronger).
    bool right_associative; ///< Whether the operator associates right-to-left.
    bool overloadable = true; ///< Whether user code may overload this operator.
  };

  extern std::unordered_map<std::string, const OperatorInfo> builtin_binary; ///< Maps operator symbol to info, for builtin binary operators.

  extern std::unordered_map<std::string, const OperatorInfo> builtin_unary; ///< Maps operator symbol to info, for builtin unary operators.

  extern const OperatorInfo generic_binary; ///< Operator info for a generic (not built-in) binary operator.

  extern const OperatorInfo generic_unary; ///< Operator info for a generic (not built-in) unary operator.
}
