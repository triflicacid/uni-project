#include "symbol_reference.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "value/symbol.hpp"
#include "message_helper.hpp"
#include "operators/builtins.hpp"
#include "ast/types/array.hpp"

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
  // check if already done
  if (value().is_lvalue()) return true;

  // resolve SymbolRef
  auto symbol = value_->get_symbol_ref()->resolve(*this, ctx.messages, type_hint());
  if (!symbol) return false;
  value_->lvalue(std::move(symbol));

  return true;
}

bool lang::ast::SymbolReferenceNode::generate_code(lang::Context& ctx) {
  // get attached symbol
  assert(value_->is_lvalue());
  const value::Symbol* symbol = value().lvalue().get_symbol();
  assert(symbol != nullptr);

  // ensure the symbol is defined
  if (!symbol->get().define(ctx)) return false;

  // get the symbol's type
  auto& type = symbol->type();

  // special case: if symbol is an array (non-zero), don't load its value, but its address
//  if (auto array_type = type.get_array()) {
//    if (array_type->size() > 0) {
//      // implicitly get the address of
//      bool success = ops::address_of(ctx, symbol->get(), *value_);
//      assert(success); // should always be able to get the address of
//      return true;
//    }
//  }

  // load into registers
  if (type.size() == 0) return true;
  const memory::Ref ref = ctx.reg_alloc_manager.find_or_insert(symbol->get());
  value_->rvalue(std::make_unique<value::RValue>(type, ref));
  return true;
}
