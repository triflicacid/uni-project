#pragma once

#include "messages/list.hpp"
#include "lexer/token.hpp"

namespace lang::symbol {
  class Symbol;
}

namespace lang::ast::type {
  class Node;
}

namespace lang::value {
  class Value;
}

namespace lang::util {
  // generate message for failing to resolve a symbol
  std::unique_ptr<message::Message> error_symbol_not_found(const message::MessageGenerator& source, const std::string& name);

  // generate message for a type mismatch: "cannot assign/convert <a> to <b>"
  std::unique_ptr<message::Message> error_type_mismatch(const message::MessageGenerator& source, const ast::type::Node& a, const ast::type::Node& b, bool is_assignment);

  // error for `type(a) a has no member b`
  std::unique_ptr<message::Message> error_no_member(const message::MessageGenerator& source, const ast::type::Node& type_a, const std::string& a, const std::string& b);

  // generate error for failing to resolve an overloaded symbol due to insufficient information NOT because we have no matches
  std::unique_ptr<message::Message> error_insufficient_info_to_resolve_symbol(const message::MessageGenerator& source, const std::string& name);

  // print note messages for each candidate overload
  void note_candidates(const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates, message::List& messages);

  // generate error for failing to match symbol to type hint
  std::unique_ptr<message::Message> error_cannot_match_type_hint(const message::MessageGenerator& source, const std::string& name, const ast::type::Node& type_hint);

  // generate error for using the special discard operator '_' as a name
  std::unique_ptr<message::Message> error_underscore_bad_use(const message::MessageGenerator& source);
}
