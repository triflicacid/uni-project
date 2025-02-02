#include <cassert>
#include "symbol_declaration.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "assembly/directive.hpp"
#include "symbol/registry.hpp"
#include "ast/types/graph.hpp"
#include "ast/types/wrapper.hpp"
#include "message_helper.hpp"
#include "assembly/create.hpp"
#include "ast/types/const.hpp"

std::string lang::ast::SymbolDeclarationNode::node_name() const {
  switch (category_) {
    case Variable:
      return "symbol declaration";
    case Argument:
      return "argument";
    case Constant:
      return "constant declaration";
  }
}

const lang::ast::type::Node& lang::ast::SymbolDeclarationNode::type() const {
  assert(type_.has_value());
  return type_.value();
}

std::ostream &lang::ast::SymbolDeclarationNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "let " << name_.image << ": ";
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
  os << SHELL_GREEN << name_.image << SHELL_RESET << ": " << SHELL_CYAN;
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

  // if no type AND no way to deduce it, error
  if (!type_.has_value() && !assignment_.has_value()) {
    auto msg = name_.generate_message(message::Error);
    msg->get() << "cannot deduce type - expected explicit type or initialiser";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // if need be, get value to be assign
  std::unique_ptr<value::Value> assignment_value = nullptr;
  if (assignment_.has_value()) {
    assignment_value = assignment_->get()->get().get_value(ctx);
    if (!assignment_value) return false;
    const auto& type = assignment_value->type();

    // if we have declared symbol to be a type, they must match
    if (type_.has_value() && !type::graph.is_subtype(type.id(), type_->get().id())) {
      ctx.messages.add(util::error_type_mismatch(name_, type, type_.value(), true));
      return false;
    }

    type_ = type;
  }

  // define symbol
  symbol::Registry registry;
  auto maybe_id = symbol::create_variable(registry, category_ == Argument ? symbol::Category::Argument : symbol::Category::Ordinary, name_, type_.value(), ctx.messages);
  if (!maybe_id.has_value()) return false;
  ctx.symbols.insert(registry);
  id_ = maybe_id.value();

  // if no assignment, we're done
  if (!assignment_.has_value()) return true;

  // get location of expr
  const auto maybe_expr = ctx.reg_alloc_manager.get_recent();
  assert(maybe_expr.has_value());
  // coerce into correct type (this is safe as subtyping checked)
  const memory::Ref& expr = ctx.reg_alloc_manager.guarantee_datatype(maybe_expr.value(), type_.value().get().get_asm_datatype());

  // load expr value into symbol
  ctx.symbols.assign_symbol(id_, expr.offset);
  return true;
}
