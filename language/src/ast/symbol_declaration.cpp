#include <cassert>
#include "symbol_declaration.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "assembly/directive.hpp"
#include "symbol/registry.hpp"
#include "ast/types/graph.hpp"
#include "message_helper.hpp"
#include "assembly/create.hpp"

const lang::ast::type::Node& lang::ast::SymbolDeclarationNode::type() const {
  assert(type_.has_value());
  return type_.value();
}

std::ostream &lang::ast::SymbolDeclarationNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "let " << token_.image << ": ";
  if (type_.has_value()) {
    type_.value().get().print_code(os);
  } else {
    os << "?";
  }

  if (assignment_.has_value()) {
    os << " = ";
    assignment_.value()->print_code(os);
  }

  return os;
}

std::ostream &lang::ast::SymbolDeclarationNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << token_.image << SHELL_RESET << ": " << SHELL_CYAN;
  if (type_.has_value()) {
    type_.value().get().print_code(os);
  } else {
    os << "?";
  }
  os << SHELL_RESET;

  if (assignment_.has_value()) {
    os << std::endl;
    assignment_.value()->print_tree(os, indent_level + 1);
  }

  return os;
}

bool lang::ast::SymbolDeclarationNode::collate_registry(message::List& messages, symbol::Registry& registry) {
  // symbols are not available prior to definition, so do nothing here
  return true;
}

bool lang::ast::SymbolDeclarationNode::process(lang::Context& ctx) {
  // process the assignment body
  if (assignment_.has_value()) {
    if (!assignment_.value()->process(ctx)) return false;
  }

  // deduce type if required
  if (!type_.has_value()) {
    // this requires an assignment body
    if (!assignment_.has_value()) {
      auto msg = token_.generate_message(message::Error);
      msg->get() << "cannot deduce type - expected explicit type or initialiser";
      ctx.messages.add(std::move(msg));
      return false;
    }

    type_ = assignment_.value()->type();
  } else {
    // check types match (don't do this above as we *know*, via deduction, that types match)
    if (!type::graph.is_subtype(assignment_.value()->type().id(), type_.value().get().id())) {
      ctx.messages.add(util::error_type_mismatch(token_, assignment_.value()->type(), type_.value(), true));
      return false;
    }
  }

  // TODO type coercion

  // define symbol
  symbol::Registry registry;
  auto maybe_id = symbol::create_variable(registry, arg_ ? symbol::Category::Argument : symbol::Category::Ordinary, token_, type_.value(), ctx.messages);
  if (!maybe_id.has_value()) return false;
  ctx.symbols.insert(registry);
  id_ = maybe_id.value();

  // get location of symbol
  const auto symbol_loc = ctx.symbols.locate(id_);
  assert(symbol_loc.has_value());

  // load expr value into symbol
  auto expr_ref = ctx.reg_alloc_manager.get_recent();
  assert(expr_ref.has_value());
  expr_ref = ctx.reg_alloc_manager.guarantee_register(expr_ref.value());
  ctx.symbols.assign_symbol(id_, expr_ref.value().offset);
  return true;
}
