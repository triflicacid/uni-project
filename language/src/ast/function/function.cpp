#include "function.hpp"
#include "../block.hpp"
#include "context.hpp"
#include "assembly/create.hpp"
#include "message_helper.hpp"

lang::ast::FunctionNode::FunctionNode(lang::lexer::Token token, lexer::Token name, const type::FunctionNode& type,
                                      std::deque<std::unique_ptr<SymbolDeclarationNode>> params,
                                      std::optional<std::unique_ptr<BlockNode>> body)
    : FunctionBaseNode(std::move(token), std::move(name), type, std::move(params)), body_(std::move(body)) {}

std::ostream& lang::ast::FunctionNode::print_code(std::ostream& os, unsigned int indent_level) const {
  FunctionBaseNode::print_code(os, indent_level);

  // print body
  if (body_.has_value()) {
    return body_.value()->print_code(os, indent_level);
  }

  return os << ";";
}

std::ostream& lang::ast::FunctionNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  FunctionBaseNode::print_tree(os, indent_level);

  // print body
  if (body_.has_value()) {
    os << std::endl;
    body_.value()->print_tree(os, indent_level + 1);
  }

  return os;
}

bool lang::ast::FunctionNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  // call parent's method
  if (!FunctionBaseNode::collate_registry(messages, registry))
    return false;

  // ensure our body has no scope
  if (body_.has_value()) {
    registry_ = std::make_unique<symbol::Registry>();
    body_.value()->add_new_scope(false);
    if (!body_.value()->collate_registry(messages, *registry_))
      return false;
  }

  return true;
}

bool lang::ast::FunctionNode::_process(lang::Context& ctx) {
  // if no body, we are fine
  if (!body_.has_value()) return true;

  // add registry to our scope
  ctx.symbols.insert(*registry_);
  registry_ = nullptr;

  // finally, process our body
  return body_->get()->process(ctx);
}

bool lang::ast::FunctionNode::always_returns() const {
  return body_.has_value() && body_.value()->always_returns();
}
