#include "symbol_reference.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "value/symbol.hpp"
#include "message_helper.hpp"

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

  value_ = value::symbol_ref(symbol_); // just reference, you guessed it, symbol reference!
  return true;
}

bool lang::ast::SymbolReferenceNode::resolve_lvalue(Context& ctx) {
  // check if already done
  if (value().is_lvalue()) return true;

  // resolve SymbolRef
  auto symbol = value_->get_symbol_ref()->resolve(ctx, *this, true, type_hint());
  if (!symbol) return false;
  value_->lvalue(std::move(symbol));

  return true;
}

bool lang::ast::SymbolReferenceNode::resolve_rvalue(Context& ctx) {
  // ensure symbol is resolved
  if (!resolve_lvalue(ctx)) return false;

  // get attached symbol
  const value::Symbol* symbol = value().lvalue().get_symbol();
  assert(symbol != nullptr);

  // load into registers
  if (symbol->type().size() == 0) return true;
  const memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(symbol->get());
  value_->rvalue(std::make_unique<value::RValue>(symbol->type(), ref));
  return true;
}
