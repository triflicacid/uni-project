#include <cassert>
#include "function_base.hpp"
#include "types/function.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "assembly/create.hpp"
#include "symbol_declaration.hpp"

lang::ast::FunctionBaseNode::FunctionBaseNode(lang::lexer::Token token, lexer::Token name, const type::FunctionNode& type,
                                              std::deque<std::unique_ptr<SymbolDeclarationNode>> params)
    : Node(std::move(token)), name_(std::move(name)), type_(type), params_(std::move(params)) {}

bool lang::ast::FunctionBaseNode::validate_params(message::List& messages) {
  // check for duplicate names
  std::set<std::string> names;
  std::map<std::string, std::reference_wrapper<const lexer::Token>> tokens;
  std::string name;
  for (const auto& param : params_) {
    // extract name & check if it exists already
    name = name_.image;
    if (names.contains(name)) {
      auto msg = param->generate_message(message::Error);
      msg->get() << "duplicate parameter name " << name;
      messages.add(std::move(msg));

      msg = tokens.find(name)->second.get().generate_message(message::Note);
      msg->get() << "parameter first found here";
      messages.add(std::move(msg));
      return false;
    }

    // cache parameter name & its location (token)
    names.insert(name);
    tokens.insert({name, param->token_start()});
  }

  return true;
}

std::ostream& lang::ast::FunctionBaseNode::print_code(std::ostream& os, unsigned int indent_level) const {
  // print comma-separated bracketed list of parameter types
  indent(os, indent_level) << block_prefix() << "(";
  for (int i = 0; i < params_.size(); i++) {
    os << params_[i]->name().image << ": ";
    type_.arg(i).print_code(os);
    if (i < params_.size() - 1) os << ", ";
  }
  os << ") ";

  // if provided, print return type after an arrow
  if (auto& returns = type_.returns(); returns.id() != type::unit.id()) {
    os << "-> ";
    returns.print_code(os);
  }

  return os;
}

std::ostream& lang::ast::FunctionBaseNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level)  << SHELL_GREEN << name_.image << SHELL_RESET;

  // print arguments
  for (auto& param : params_) {
    os << std::endl;
    indent(os, indent_level);
    param->print_tree(os, indent_level + 1);
  }

  return os;
}

bool lang::ast::FunctionBaseNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  // attempt to register the symbol
  symbol::VariableOptions options{
    .token = name_,
    .type = type_,
    .category = symbol::Category::Function,
  };
  auto maybe_id = symbol::create_variable(registry, options, messages);
  if (!maybe_id.has_value()) return false;
  id_ = maybe_id.value();

  // create local registry
  registry_ = std::make_unique<symbol::Registry>();

  // collate parameter definitions
  for (auto& param : params_) {
    param->set_category(SymbolDeclarationNode::Argument); // ensure they are arguments
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

  // symbol has already been inserted, so lookup the associated block
  auto maybe_store = ctx.symbols.locate(id_);
  assert(maybe_store.has_value() && maybe_store->get().type == memory::StorageLocation::Block);
  const assembly::BasicBlock& function_block = maybe_store->get().block;

  // change cursor position
  const assembly::BasicBlock& previous = ctx.program.current();
  ctx.program.select(function_block);

  // enter the function
  ctx.symbols.enter_function(*this);

  // process as child directs
  if (!_process(ctx)) return false;

  // restore cursor to previous block and exit function
  ctx.symbols.exit_function();
  ctx.program.select(previous);

  return true;
}
