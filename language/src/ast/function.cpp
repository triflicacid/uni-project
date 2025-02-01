#include "function.hpp"
#include "symbol_declaration.hpp"
#include "block.hpp"
#include "types/function.hpp"
#include "context.hpp"
#include "assembly/create.hpp"
#include "message_helper.hpp"

lang::ast::FunctionNode::FunctionNode(lang::lexer::Token token, const type::FunctionNode& type,
                                      std::deque<std::unique_ptr<SymbolDeclarationNode>> params,
                                      std::optional<std::unique_ptr<BlockNode>> body)
    : FunctionBaseNode(std::move(token), type, std::move(params)), body_(std::move(body)) {}

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
    indent(os, indent_level);
    body_.value()->print_tree(os, indent_level + 1);
  }

  return os;
}

bool lang::ast::FunctionNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  // call parent's method
  if (!FunctionBaseNode::collate_registry(messages, registry))
    return false;

  // collate function body with our registry
  if (body_.has_value()) {
    body_.value()->add_new_scope(false);
    if (!body_.value()->collate_registry(messages, *registry_))
      return false;
  }

  return true;
}

bool lang::ast::FunctionNode::_process(lang::Context& ctx) {
  // insert our registry into the symbol table
  ctx.symbols.push();
  ctx.symbols.insert(*registry_);

  // if we have no body, this is *really* easy
  if (body_.has_value()) {
    if (!body_.value()->process(ctx)) return false;
  } else {
    // just return TODO type??
    ctx.program.current().add(assembly::create_return());
  }

  // remove function scope
  ctx.symbols.pop();

  return true;
}
