#pragma once

#include "messages/message.hpp"

namespace lang::conf {
  extern bool debug;

  // lint - do linting?
  extern bool lint;

  // lint - note or warning
  extern message::Level lint_level;

  // function declaration behaviour - enforce existence or define as empty (placeholder)
  extern bool function_placeholder;

  // indent code inside basic blocks?
  extern bool indent_asm_code;

  namespace bools {
    extern std::string true_string;
    extern std::string false_string;
  }
}
