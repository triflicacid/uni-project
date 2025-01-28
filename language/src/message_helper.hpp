#pragma once

#include "messages/list.hpp"

namespace lang::symbol {
  class SymbolTable;
}

namespace lang::util {
  // check local symbol scope - used before removal, checks unused symbols etc...
  void check_local_scope(const symbol::SymbolTable& symbols, message::List &messages);

  // generate message for failing to find a candidate -- provide options
}
