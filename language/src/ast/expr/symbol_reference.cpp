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

bool lang::ast::expr::SymbolReferenceNode::resolve_lvalue(Context& ctx, value::Value& value) const {
  // check if already done
  if (value.is_lvalue()) return true;

  // resolve SymbolRef
  auto symbol = value.get_symbol_ref()->resolve(ctx, *this, true);
  if (!symbol) return false;
  value.lvalue(std::move(symbol));

  return true;
}

bool lang::ast::expr::SymbolReferenceNode::resolve_rvalue(Context& ctx, value::Value& value) const {
  // ensure symbol is resolved
  if (!resolve_lvalue(ctx, value)) return false;

  // get attached symbol
  const value::Symbol* symbol = value.lvalue().get_symbol();
  assert(symbol != nullptr);

  // load into registers
  if (symbol->type().size() == 0) return true;
  const memory::Ref ref = ctx.reg_alloc_manager.find(static_cast<const symbol::Variable&>(symbol->get()));
  value.rvalue(std::make_unique<value::RValue>(symbol->type(), ref));
  return true;
}
