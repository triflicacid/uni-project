#pragma once

#include "messages/message.hpp"

namespace lang::conf {
  extern bool debug;

  // lint - do linting?
  extern bool lint;

  // lint - note or warning
  extern message::Level lint_level;

  namespace bools {
    extern std::string true_string;
    extern uint8_t true_value;
    extern std::string false_string;
    extern uint8_t false_value;
  }
}
