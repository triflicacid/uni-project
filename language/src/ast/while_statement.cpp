#include "while_statement.hpp"
#include "ast/types/graph.hpp"
#include "ast/types/bool.hpp"
#include "message_helper.hpp"
#include "context.hpp"
#include "assembly/create.hpp"

static unsigned int current_id = 0;

lang::ast::WhileStatementNode::WhileStatementNode(lang::lexer::Token token, std::unique_ptr<Node> guard, std::unique_ptr<Node> body)
  : Node(std::move(token)), guard_(std::move(guard)), body_(std::move(body)), id_(current_id++) {}

std::ostream& lang::ast::WhileStatementNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "while ";
  guard_->print_code(os, indent_level + 1) << " ";
  body_->print_code(os, indent_level + 1);
  return os;
}

std::ostream& lang::ast::WhileStatementNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);

  os << std::endl;
  guard_->print_tree(os, indent_level + 1);

  os << std::endl;
  body_->print_tree(os, indent_level + 1);
  return os;
}

bool lang::ast::WhileStatementNode::always_returns() const {
  return guard_->always_returns() || body_->always_returns();
}

bool lang::ast::WhileStatementNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  return guard_->collate_registry(messages, registry)
    && body_->collate_registry(messages, registry);
}

bool lang::ast::WhileStatementNode::process(lang::Context& ctx) {
  // process guard
  if (!guard_->process(ctx) || !body_->process(ctx)) return false;

  // check that the guard is a Boolean
  if (!guard_->resolve(ctx)) return false;
  auto& guard_value = guard_->value();
  if (!type::graph.is_subtype(guard_value.type().id(), type::boolean.id())) {
    ctx.messages.add(util::error_type_mismatch(*guard_, guard_value.type(), ast::type::boolean, false));
    return false;
  }

  return true;
}

bool lang::ast::WhileStatementNode::generate_code(lang::Context& ctx) const {
  // resolve our body
  if (!body_->resolve(ctx)) return false;

  // create the loop's blocks
  auto guard_block = assembly::BasicBlock::labelled("whileguard_" + std::to_string(id_));
  auto& guard_ref = *guard_block;
  auto body_block = assembly::BasicBlock::labelled("whilebody_" + std::to_string(id_));
  auto& body_ref = *body_block;
  auto after_block = assembly::BasicBlock::labelled("endwhile_" + std::to_string(id_));

  // create contexts - one for the guard (if), one for the loop body
  control_flow::ConditionalContext cond_ctx{*body_block, *after_block};
  control_flow::LoopContext loop_ctx{*guard_block, *after_block};

  // generate the conditional guard
  ctx.program.insert(assembly::Position::Next, std::move(guard_block));
  guard_->conditional_context(cond_ctx);
  if (!guard_->generate_code(ctx)) return false;
  if (!cond_ctx.handled && !cond_ctx.generate_branches(ctx, *guard_, guard_->value())) return false;

  // generate the body
  ctx.program.insert(assembly::Position::Next, std::move(body_block));
  body_->loop_context(loop_ctx);
  if (!body_->generate_code(ctx)) return false;

  // add branch back to the guard (form a loop)
  body_ref.add(assembly::create_branch(assembly::Arg::label(guard_ref)));

  // create the `after` block for when the loop exits
  ctx.program.insert(assembly::Position::Next, std::move(after_block));

  return true;
}
