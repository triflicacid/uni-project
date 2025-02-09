#pragma once

#include "messages/message.hpp"

namespace lang::conf {
  extern bool debug;

  // lint - do linting?
  extern bool lint;

  // lint - note or warning
  extern message::Level lint_level;

  // function declaration behaviour - enforce existence or define as empty
  extern bool func_decl_generate_shell;

  namespace bools {
    extern std::string true_string;
    extern std::string false_string;
  }
}
