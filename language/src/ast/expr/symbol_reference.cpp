#include "symbol_reference.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "ast/types/namespace.hpp"

std::ostream &lang::ast::expr::SymbolReferenceNode::print_code(std::ostream &os, unsigned int indent_level) const {
  return os << token_.image;
}

std::ostream &lang::ast::expr::SymbolReferenceNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  return os << " " SHELL_GREEN << token_.image << SHELL_RESET;
}

std::optional<std::reference_wrapper<lang::symbol::Symbol>>
lang::ast::expr::SymbolReferenceNode::select_candidate(lang::Context& ctx, const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates) const {
  // if >1 candidates, we have no information to decide
  if (candidates.size() > 1) {
    throw std::runtime_error("unable to select candidate for non-function symbol reference: multiple provided");
  }

  return candidates.front();
}

bool lang::ast::expr::SymbolReferenceNode::process(lang::Context& ctx) {
  // check if this symbol exists
  const std::string symbol_name = token_.image;
  auto& candidates = ctx.symbols.find(symbol_name);

  // if no candidates, this symbol does not exist
  if (candidates.empty()) {
    auto msg = token_.generate_message(message::Error);
    msg->get() << "unable to resolve reference to symbol '" << symbol_name << "'";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // select suitable candidate, exit if none found
  auto candidate = select_candidate(ctx, candidates);
  if (!candidate.has_value()) {
    return false;
  }

  // record the symbol used
  symbol_ = std::move(candidate);

  // TODO for demonstration only
  load(ctx);

  return true;
}

bool lang::ast::expr::SymbolReferenceNode::load(lang::Context& ctx) const {
  // record reference
  symbol_->get().inc_ref();

  // if this is not a namespace, we know it is a variable
  if (auto& type = symbol_->get().type(); type.id() == ast::type::name_space.id()) return true;

  // move into a register
  ctx.reg_alloc_manager.find(static_cast<symbol::Variable&>(symbol_->get()));
  return true;
}
