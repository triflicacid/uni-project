#pragma once

#include "messages/list.hpp"
#include "lexer/token.hpp"

namespace lang::symbol {
  class SymbolTable;
}

namespace lang::ast::type {
  class Node;
}

namespace lang::value {
  class Value;
}

namespace lang::util {
  // generate message for failing to resolve a symbol
  std::unique_ptr<message::Message> error_unresolved_symbol(const message::MessageGenerator& source, const std::string& name);

  // generate message for a type mismatch: "cannot assign/convert <a> to <b>"
  std::unique_ptr<message::Message> error_type_mismatch(const message::MessageGenerator& source, const ast::type::Node& a, const ast::type::Node& b, bool is_assignment);

  // error for `type(a) a has no member b`
  std::unique_ptr<message::Message> error_no_member(const message::MessageGenerator& source, const ast::type::Node& type_a, const std::string& a, const std::string& b);

  // generate error for failing to resolve an overloaded symbol due to insufficient information NOT because we have no matches
  std::unique_ptr<message::Message> error_insufficient_info_to_resolve_symbol(const message::MessageGenerator& source, const std::string& name);

  // generate error if the given Value is not a concrete symbol (i.e., value::Symbol), or return nullptr
  std::unique_ptr<message::Message> check_concrete_symbol(const message::MessageGenerator& source, const value::Value& value);
}
