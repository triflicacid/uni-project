#pragma once

#include "messages/message.hpp"

namespace lang::conf {
  extern bool debug;

  // lint - do linting?
  extern bool lint;

  // lint - note or warning
  extern message::Level lint_level;
}
