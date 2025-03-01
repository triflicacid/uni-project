#pragma once

#include "messages/list.hpp"
#include "lexer/token.hpp"
#include "optional_ref.hpp"

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

  // type mismatch error for if-else statement
  // state if `else is implicit or not by including its type
  void error_if_statement_mismatch(message::List& messages, const message::MessageGenerator& if_source, const message::MessageGenerator& then_source, const ast::type::Node& then_type, const message::MessageGenerator& else_source, optional_ref<const ast::type::Node> else_type);

  // error for `type(a) a has no member b`
  std::unique_ptr<message::Message> error_no_member(const message::MessageGenerator& source, const ast::type::Node& type_a, const std::string& a, const std::string& b);

  // generate error for failing to resolve an overloaded symbol due to insufficient information NOT because we have no matches
  std::unique_ptr<message::Message> error_ambiguous_reference(const message::MessageGenerator& source, const std::string& name);

  // print note messages for each candidate overload
  void note_candidates(const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates, message::List& messages);

  // generate error for failing to match symbol to type hint
  std::unique_ptr<message::Message> error_cannot_match_type_hint(const message::MessageGenerator& source, const std::string& name, const ast::type::Node& type_hint);

  // generate error for using the special discard operator '_' as a name
  std::unique_ptr<message::Message> error_underscore_bad_use(const message::MessageGenerator& source);

  // error for expected l-- or r-value
  std::unique_ptr<message::Message> error_expected_lrvalue(const message::MessageGenerator& source, const ast::type::Node& type, bool expected_lvalue);

  // add `while evaluating...` note. with options to add extra messages
  std::unique_ptr<message::Message> note_while_evaluating(const message::MessageGenerator& source, std::optional<std::string> addendum);

  // error that a literal cannot be stored in the given type
  std::unique_ptr<message::Message> error_literal_bad_type(const message::MessageGenerator& source, const std::string& literal, const ast::type::Node& type);
}
