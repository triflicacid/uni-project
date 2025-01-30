#pragma once

#include "messages/list.hpp"
#include "lexer/token.hpp"

namespace lang::symbol {
  class SymbolTable;
}

namespace lang::ast::type {
  class Node;
}

namespace lang::util {
  // check local symbol scope - used before removal, checks unused symbols etc...
  void check_local_scope(const symbol::SymbolTable& symbols, message::List &messages);

  // generate message for a type mismatch: "cannot assign/convert <a> to <b>"
  std::unique_ptr<message::Message> error_type_mismatch(const lexer::Token& token, const ast::type::Node& a, const ast::type::Node& b, bool is_assignment);
}
