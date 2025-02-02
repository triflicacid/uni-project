#include "symbol_reference.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "value/symbol.hpp"
#include "message_helper.hpp"

std::ostream &lang::ast::expr::SymbolReferenceNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << token_start().image;
}

std::ostream &lang::ast::expr::SymbolReferenceNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  return os << SHELL_GREEN << token_start().image << SHELL_RESET;
}

bool lang::ast::expr::SymbolReferenceNode::process(lang::Context& ctx) {
  return true;
}

std::unique_ptr<lang::value::Value> lang::ast::expr::SymbolReferenceNode::get_value(lang::Context& ctx) const {
  // just reference, you guessed it, symbol reference!
  return std::make_unique<value::SymbolRef>(token_start().image);
}

std::unique_ptr<lang::value::Value> lang::ast::expr::SymbolReferenceNode::load(lang::Context& ctx) const {
  auto value = get_value(ctx);
  value = value->get_symbol_ref()->resolve(ctx, *this, true);
  if (!value) return nullptr;

  // exit if we are not computable (e.g., namespace)
  if (!value->is_computable()) return value;

  // load into a register
  const memory::Ref ref = ctx.reg_alloc_manager.find(static_cast<const symbol::Variable&>(value->get_symbol()->get()));
  value->set_ref(ref);
  return value;
}
