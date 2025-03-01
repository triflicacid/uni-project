#include "symbol_reference.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "value/future.hpp"
#include "message_helper.hpp"
#include "operators/builtins.hpp"
#include "ast/types/array.hpp"
#include "assembly/create.hpp"

std::ostream &lang::ast::SymbolReferenceNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << symbol_;
}

std::ostream &lang::ast::SymbolReferenceNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  return os << SHELL_GREEN << symbol_ << SHELL_RESET;
}

bool lang::ast::SymbolReferenceNode::process(lang::Context& ctx) {
  // we cannot reference special variable '_'
  if (symbol_ == "_") {
    auto msg = generate_message(message::Error);
    msg->get() << "special discard symbol cannot be referenced";
    ctx.messages.add(std::move(msg));
    return false;
  }

  value_ = value::symbol_ref(symbol_, ctx.symbols); // just reference, you guessed it, symbol reference!
  return true;
}

bool lang::ast::SymbolReferenceNode::resolve(Context& ctx) {
  // resolve SymbolRef
  assert(value_->get_symbol_ref());
  return static_cast<value::SymbolRef&>(*value_).resolve(*this, ctx.messages, type_hint());
}
