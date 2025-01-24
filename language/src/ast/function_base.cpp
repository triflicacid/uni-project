#include "function_base.hpp"
#include "types/function.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "assembly/create.hpp"
#include "symbol_declaration.hpp"

lang::ast::FunctionBaseNode::FunctionBaseNode(lang::lexer::Token token, std::unique_ptr<type::FunctionNode> type,
                                              std::deque<std::unique_ptr<SymbolDeclarationNode>> params)
    : Node(std::move(token)), type_(std::move(type)), params_(std::move(params)) {}

bool lang::ast::FunctionBaseNode::validate_params(message::List& messages) {
  // check for duplicate names
  std::set<std::string> names;
  std::map<std::string, std::reference_wrapper<const lexer::Token>> tokens;
  std::string name;
  for (const auto& param : params_) {
    // extract name & check if it exists already
    name = param->token().image;
    if (names.contains(name)) {
      auto msg = param->token().generate_message(message::Error);
      msg->get() << "duplicate parameter name " << name;
      messages.add(std::move(msg));

      msg = tokens.find(name)->second.get().generate_message(message::Note);
      msg->get() << "parameter first found here";
      messages.add(std::move(msg));
      return false;
    }

    // cache parameter name & its location (token)
    names.insert(name);
    tokens.insert({name, param->token()});
  }

  return true;
}

std::ostream& lang::ast::FunctionBaseNode::print_code(std::ostream& os, unsigned int indent_level) const {
  // print comma-separated bracketed list of parameter types
  indent(os, indent_level) << block_prefix() << "(";
  for (int i = 0; i < params_.size(); i++) {
    os << params_[i]->token().image << ": ";
    type_->arg(i).print_code(os, 0);
    if (i < params_.size() - 1) os << ", ";
  }
  os << ") ";

  // if provided, print return type after an arrow
  if (auto returns = type_->returns(); returns.has_value()) {
    os << "-> ";
    returns.value().get().print_code(os, 0);
  }

  return os;
}

std::ostream& lang::ast::FunctionBaseNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level)  << SHELL_GREEN << token_.image << SHELL_RESET;

  // print arguments
  for (auto& param : params_) {
    os << std::endl;
    indent(os, indent_level);
    param->print_tree(os, indent_level + 1);
  }

  return os;
}

bool lang::ast::FunctionBaseNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  // create local registry
  registry_ = std::make_unique<symbol::Registry>();

  // collate parameter definitions
  for (auto& param : params_) {
    if (!param->collate_registry(messages, *registry_)) {
      return false;
    }
  }

  return true;
}

bool lang::ast::FunctionBaseNode::process(lang::Context& ctx) {
  // first: validation
  if (!validate_params(ctx.messages)) {
    return false;
  }

  // create a symbol to represent us
  auto symbol = std::make_unique<symbol::Variable>(token_, *type_);

  // create a new block for us
  const std::string label = "func_" + std::to_string(symbol->id());
  auto block = assembly::BasicBlock::labelled(label);
  block->comment() << token_.image;
  type_->print_code(block->comment());

  // insert ourself into the scope
  ctx.symbols.insert(std::move(symbol), *block);

  // insert block into program
  const assembly::BasicBlock& previous = ctx.program.current();
  ctx.program.insert(assembly::Position::After, std::move(block));

  // process as child directs
  if (!_process(ctx)) return false;

  // restore cursor to previous block
  ctx.program.select(previous);

  return true;
}
