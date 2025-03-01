#include "message_helper.hpp"
#include "ast/types/node.hpp"
#include "symbol/symbol.hpp"
#include "config.hpp"
#include "ast/types/unit.hpp"

std::unique_ptr<message::Message> lang::util::error_type_mismatch(const message::MessageGenerator& source, const lang::ast::type::Node& a, const lang::ast::type::Node& b, bool is_assignment) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "type mismatch: cannot " << (is_assignment ? "assign" : "convert") << " ";
  a.print_code(msg->get());
  msg->get() << " to ";
  b.print_code(msg->get());
  return msg;
}

std::unique_ptr<message::Message>
lang::util::error_symbol_not_found(const message::MessageGenerator& source, const std::string& name) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "symbol '" << name << "' does not exist";
  return msg;
}

std::unique_ptr<message::Message>
lang::util::error_no_member(const message::MessageGenerator& source, const lang::ast::type::Node& type_a, const std::string& a, const std::string& b) {
  auto msg = source.generate_message(message::Error);
  type_a.print_code(msg->get());
  msg->get() << " '" << a << "' has no member '" << b << "'";
  return msg;
}

std::unique_ptr<message::Message>
lang::util::error_ambiguous_reference(const message::MessageGenerator& source, const std::string& name) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "ambiguous reference to overloaded symbol '" << name << "'";
  return msg;
}

std::unique_ptr<message::Message> lang::util::error_underscore_bad_use(const message::MessageGenerator& source) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "special discard symbol cannot be used here";
  return msg;
}

void lang::util::note_candidates(const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates, message::List& messages) {
  for (auto& candidate : candidates) {
    auto msg = candidate.get().token().generate_message(message::Note);
    msg->get() << "overload ";
    candidate.get().type().print_code(msg->get());
    msg->get() << " defined here";
    messages.add(std::move(msg));
  }
}

std::unique_ptr<message::Message>
lang::util::error_cannot_match_type_hint(const message::MessageGenerator& source, const std::string& name, const lang::ast::type::Node& type_hint) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "no overloads for '" << name << "' matching type hint ";
  type_hint.print_code(msg->get());
  return msg;
}

std::unique_ptr<message::Message> lang::util::error_expected_lrvalue(const message::MessageGenerator& source, const ast::type::Node& type, bool expected_lvalue) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "expected " << (expected_lvalue ? 'l' : 'r') << "value, got ";
  type.print_code(msg->get());
  return msg;
}

std::unique_ptr<message::Message> lang::util::note_while_evaluating(const message::MessageGenerator& source, std::optional<std::string> addendum) {
  auto msg = source.generate_message(message::Note);
  msg->get() << "while evaluating " << (addendum.has_value() ? addendum.value() : "this");
  return msg;
}

void lang::util::error_if_statement_mismatch(message::List& messages, const message::MessageGenerator& if_source,
                                             const message::MessageGenerator& then_source,
                                             const lang::ast::type::Node& then_type,
                                             const message::MessageGenerator& else_source,
                                             optional_ref<const lang::ast::type::Node> else_type) {
  auto msg = if_source.generate_message(message::Error);
  if (else_type.has_value()) {
    msg->get() << "type mismatch: the blocks of an if statement must return the same type, got ";
    then_type.print_code(msg->get());
    msg->get() << " and ";
    else_type->get().print_code(msg->get());
  } else {
    msg->get() << "missing 'else' branch in an 'if' expression (cannot match ";
    then_type.print_code(msg->get());
    msg->get() << " and ";
    ast::type::unit.print_code(msg->get()) << ")";
  }
  messages.add(std::move(msg));

  msg = then_source.generate_message(message::Note);
  then_type.print_code(msg->get()) << " returned here";
  messages.add(std::move(msg));

  msg = else_source.generate_message(message::Note);
  if (else_type.has_value()) else_type->get().print_code(msg->get());
  else ast::type::unit.print_code(msg->get()) << " implicitly";
  msg->get() << " returned here";
  if (!else_type.has_value()) {
    msg->get() << " ('if' expression without 'else' evaluates to '()')";
  }
  messages.add(std::move(msg));
}

std::unique_ptr<message::Message> lang::util::error_literal_bad_type(const message::MessageGenerator& source, const std::string& literal, const ast::type::Node& type) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "literal '" << literal << "' is unable to be stored in type ";
  type.print_code(msg->get());
  return msg;
}
