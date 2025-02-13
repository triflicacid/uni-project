#include "message_helper.hpp"
#include "ast/types/node.hpp"
#include "config.hpp"

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
lang::util::error_insufficient_info_to_resolve_symbol(const message::MessageGenerator& source, const std::string& name) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "insufficient information to resolve overloaded symbol '" << name << "'";
  return msg;
}

std::unique_ptr<message::Message> lang::util::error_underscore_bad_use(const message::MessageGenerator& source) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "special discard symbol cannot be used here";
  return msg;
}
