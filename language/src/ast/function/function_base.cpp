#include <cassert>
#include "function_base.hpp"
#include "../types/function.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "assembly/create.hpp"
#include "config.hpp"
#include "message_helper.hpp"
#include "symbol/function.hpp"
#include "value/future.hpp"

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
    name = param->name().image;

    // skip if special discard symbol
    if (name == "_") continue;

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
    tokens.insert({name, param->name()});
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
    param->print_tree(os, indent_level + 1);
  }

  return os;
}

bool lang::ast::FunctionBaseNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  // name cannot be the discard symbol '_'
  if (name_.image == "_") {
    messages.add(util::error_underscore_bad_use(name_));
    return false;
  }

  // if not implemented, so some checks
  if (!is_implemented()) {
    // if we are enforcing existence, skip
    if (!lang::conf::function_placeholder) return true;

    // check is symbol already exists
    auto maybe_symbol = registry.get(name_.image, type_);

    // if we do, no need to generate code
    if (maybe_symbol.has_value()) {
      generate_code_ = false;
      return true;
    }
  }

  // check for duplicate parameters
  if (!validate_params(messages)) return false;

  // assembly option object
  symbol::VariableOptions options{
    .token = name_,
    .type = type_,
    .category = symbol::Category::Function,
    .func_origin = *this,
  };

  // attempt to register the symbol
  auto maybe_id = symbol::create_variable(registry, options, messages);
  // fail if implemented, otherwise, this is fine
  if (!maybe_id.has_value()) return !is_implemented();
  id_ = maybe_id.value();

  return true;
}

bool lang::ast::FunctionBaseNode::process(lang::Context& ctx) {
  // if declaration and not generating a shell, check that we exist
  if (!is_implemented()) {
    if (lang::conf::function_placeholder) return true;

    // lookup symbol
    const std::string name = ctx.symbols.path_name(name_.image);
    auto maybe_symbol = ctx.symbols.find(name, type());

    // check the symbol exists
    if (!maybe_symbol.has_value()) {
      // error otherwise
      std::unique_ptr<message::BasicMessage> msg = generate_message(message::Error);
      msg->get() << "function was declared but does not exist";
      ctx.messages.add(std::move(msg));

      msg = std::make_unique<message::BasicMessage>(message::Note);
      msg->get() << "function signature: ";
      type_.print_code(msg->get());
      ctx.messages.add(std::move(msg));

      msg = std::make_unique<message::BasicMessage>(message::Note);
      msg->get() << "function declarations are used to enforce the existence of a function, did you forget to include a library?";
      ctx.messages.add(std::move(msg));
      return false;
    }

    id_ = maybe_symbol->get().id();
  }

  // add function to context
  ctx.symbols.enter_function(*this);
  ctx.symbols.push();

  // process parameters - we know where they should be located
  uint64_t offset = 0; // offset from $fp
  for (const auto& param : params_) {
    // TODO if discard '_', ignore
    // requires implementation in caller as well (FunctionCallOperatorNode, OperatorOverloadNode)
    // if (param->name().image == "_") continue;

    // increase offset by our width
    auto& type = param->type();
    offset += type.size();

    // create argument symbol with this location
    auto arg = std::make_unique<symbol::Symbol>(param->name(), symbol::Category::Argument, type);
    const symbol::SymbolId id = arg->id();

    // insert into scope
    ctx.symbols.insert(std::move(arg));

    // tell symbol table its stack location
    ctx.symbols.allocate(id, memory::StorageLocation::stack(offset));
  }

  // process child
  if (!_process(ctx)) return false;

  // remove the function's environment
  ctx.symbols.pop();
  ctx.symbols.exit_function();

  // check that the function returns if the return type is not ()
  auto& return_type = type_.returns();
  if (!always_returns() && return_type != type::unit) {
    auto msg = token_end().generate_message(message::Error);
    msg->get() << "missing return statement in function returning type ";
    return_type.print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = name_.generate_message(message::Note);
    msg->get() << "enclosing function defined here";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // definition is ok, but it is done when needed
  return true;
}

std::unordered_set<int> lang::ast::FunctionBaseNode::get_args_to_ignore() const {
  std::unordered_set<int> indexes;
  for (int i = 0; i < params_.size(); i++) {
    if (params_[i]->name().image == "_")
      indexes.insert(i);
  }

  return indexes;
}

bool lang::ast::FunctionBaseNode::generate_code(lang::Context& ctx) {
  // do not define now, it will be defined when used
  if (!lang::conf::always_define_symbols) return true;
  return define(ctx);
}

bool lang::ast::FunctionBaseNode::define(lang::Context& ctx) {
  if (defined_ || !generate_code_) return true;
  defined_ = true; // only call this function once

  // allocate space for the function, as this is not done on declaration
  ctx.program.add_location(token_start().loc);
  ctx.symbols.allocate(id_);
  ctx.program.remove_location();

  // symbol has already been inserted, so lookup the associated block
  auto maybe_store = ctx.symbols.locate(id_);
  assert(maybe_store.has_value() && maybe_store->get().type == memory::StorageLocation::Block);
  const assembly::BasicBlock& function_block = maybe_store->get().block;

  // change cursor position
  const assembly::BasicBlock& previous = ctx.program.current();
  ctx.program.select(function_block);

  // if declaration (so generating a shell, or we wouldn't get here), we can skip some stuff
  if (!is_implemented()) {
    // generate `ret` statement and update $ret contents in alloc manager
    auto& return_type = type_.returns();
    std::unique_ptr<value::Value> value;
    if (return_type.size() == 0) {
      value = value::rvalue(return_type, memory::Ref::reg(constants::registers::ret));
      ctx.program.current().add(assembly::create_return()); // don't bother supplying a value as it'll never be referenced
    } else {
      value = value::literal(memory::Literal::zero(return_type),
                             memory::Ref::reg(constants::registers::ret));
      ctx.program.current().add(assembly::create_return(assembly::Arg::imm(0))); // return '0' to clear $ret;
    }
    ctx.reg_alloc_manager.update_ret(memory::Object(std::move(value)));

    ctx.program.update_line_origins(token_end().loc, 0);
    ctx.program.select(previous);
    return true;
  }

  ctx.reg_alloc_manager.save_store(false); // registers only need to be saved in asm on call
  ctx.stack_manager.push_frame(false); // ensure our offset stays the same

  // process as child directs
  if (!_generate_code(ctx)) return false;

  ctx.stack_manager.pop_frame(false);

  // exit function (propagate $ret)
  ctx.reg_alloc_manager.propagate_ret();
  ctx.reg_alloc_manager.destroy_store(false);

  // check that the function returns
  if (!always_returns()) {
    // if it doesn't, we must return `()` (this is checked in ::process)
    ctx.program.current().add(assembly::create_return());
    ctx.program.current().back().origin(token_end().loc);

    // set $ret to ()
    auto value = value::rvalue(
        type::unit,
        memory::Ref::reg(constants::registers::ret)
    );
    ctx.reg_alloc_manager.update_ret(memory::Object(std::move(value)));
  }

  // move cursor back to previous position
  ctx.program.select(previous);

  return true;
}
