#include "message_helper.hpp"
#include "symbol/table.hpp"
#include "ast/types/node.hpp"
#include "config.hpp"
#include "value/value.hpp"
#include "value/symbol.hpp"

std::unique_ptr<message::Message> lang::util::error_type_mismatch(const message::MessageGenerator& source, const lang::ast::type::Node& a, const lang::ast::type::Node& b, bool is_assignment) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "type mismatch: cannot " << (is_assignment ? "assign" : "convert") << " ";
  a.print_code(msg->get());
  msg->get() << " to ";
  b.print_code(msg->get());
  return msg;
}

std::unique_ptr<message::Message>
lang::util::error_unresolved_symbol(const message::MessageGenerator& source, const std::string& name) {
  auto msg = source.generate_message(message::Error);
  msg->get() << "unable to resolve reference to symbol '" << name << "'";
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
  msg->get() << "insufficient information to resolve overloaded symbol " << name;
  return msg;
}

std::unique_ptr<message::Message> lang::util::check_concrete_symbol(const message::MessageGenerator& source, const lang::value::Value& value) {
  if (auto ref = value.get_symbol_ref()) {
    return error_unresolved_symbol(source, ref->get());
  }

  if (!value.get_symbol()) {
    auto msg = source.generate_message(message::Error);
    msg->get() << "expected symbol, got ";
    value.type().print_code(msg->get());
    msg->get() << " value";
    return msg;
  }

  return nullptr;
}
