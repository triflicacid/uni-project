#pragma once

#include "messages/message.hpp"

/** @brief Global compiler configuration flags, set from command-line options and read throughout the compiler. */
namespace lang::conf {
  /** @brief Whether debug output is enabled. */
  extern bool debug;

  /** @brief Whether to run linting checks. */
  extern bool lint;

  /** @brief Severity to report lint findings at (note or warning). */
  extern message::Level lint_level;

  /** @brief Whether an undefined function declaration is an error (false) or is defined as an empty placeholder (true). */
  extern bool function_placeholder;

  /** @brief Whether every symbol must be defined in generated code, even if unused. */
  extern bool always_define_symbols;

  /** @brief Whether to indent generated assembly code inside basic blocks. */
  extern bool indent_asm_code;

  /** @brief String representations used for boolean literals in generated/printed output. */
  namespace bools {
    extern std::string true_string;
    extern std::string false_string;
  }
}
