#include <cassert>
#include "symbol_reference.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "ast/types/namespace.hpp"
#include "value/symbol.hpp"
#include "message_helper.hpp"

std::ostream &lang::ast::expr::SymbolReferenceNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << token_.image;
}

std::ostream &lang::ast::expr::SymbolReferenceNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  return os << SHELL_GREEN << token_.image << SHELL_RESET;
}

bool lang::ast::expr::SymbolReferenceNode::process(lang::Context& ctx) {
  return true;



  return true;
}

std::unique_ptr<lang::value::Value> lang::ast::expr::SymbolReferenceNode::get_value(lang::Context& ctx) const {
  // just reference, you guessed it, symbol reference!
  return std::make_unique<value::SymbolRef>(token_.image);
}

std::unique_ptr<lang::value::Value> lang::ast::expr::SymbolReferenceNode::load(lang::Context& ctx) const {
  // as we have been requested an rvalue, it's safe to say that the symbol should exist
  const std::string symbol_name = token_.image;
  const auto candidates = ctx.symbols.find(symbol_name);

  // if no candidates, this symbol does not exist
  if (candidates.empty()) {
    ctx.messages.add(util::error_unresolved_symbol(token_, symbol_name));
    return nullptr;
  }

  // if more than one candidate, we do not have sufficient info to choose
  if (candidates.size() > 1) {
    ctx.messages.add(util::error_insufficient_info_to_resolve_symbol(token_, token_.image));
    return nullptr;
  }

  // fetch symbol
  symbol::Symbol& symbol = candidates.front().get();
  symbol.inc_ref(); // increase reference count

  // create Value instance (symbol so lvalue)
  auto location = ctx.symbols.locate(symbol.id());
  auto value = std::make_unique<value::Symbol>(symbol, value::Options{.lvalue=location, .computable=location.has_value()});

  // exit if we are not computable (e.g., namespace)
  if (!value->is_computable()) return value;

  // load into a register
  const memory::Ref ref = ctx.reg_alloc_manager.find(static_cast<const symbol::Variable&>(symbol));
  value->set_ref(ref);
  return value;
}
