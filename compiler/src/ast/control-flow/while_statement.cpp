#include "while_statement.hpp"
#include "types/graph.hpp"
#include "types/bool.hpp"
#include "message_helper.hpp"
#include "context.hpp"
#include "assembly/create.hpp"
#include "control_flow/loop_context.hpp"
#include "types/unit.hpp"

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
  if (!guard_->process(ctx)) return false;

  // check that the guard is a Boolean
  if (!guard_->resolve(ctx)) return false;
  auto& guard_value = guard_->value();
  if (!type::graph.is_subtype(guard_value.type().id(), type::boolean.id())) {
    ctx.messages.add(util::error_type_mismatch(*guard_, guard_value.type(), type::boolean, false));
    return false;
  }

  // push a new loop context
  ctx.loops.emplace(guard_label(), end_label());

  // process our body
  if (!body_->process(ctx)) return false;

  // remove loop context
  ctx.loops.pop();

  // our body should not return anything (i.e., `()`)
  if (!body_->resolve(ctx)) return false;
  auto& value = body_->value();
  if (value.type() != type::unit) {
    std::unique_ptr<message::BasicMessage> msg = token_start().generate_message(message::Error);
    msg->get() << "type mismatch: a while loop should evaluate to unit '()', but got ";
    value.type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = std::make_unique<message::BasicMessage>(message::Note);
    msg->get() << "did you forget a semicolon at the end of an expression?";
    ctx.messages.add(std::move(msg));
    return false;
  }

  return true;
}

bool lang::ast::WhileStatementNode::generate_code(lang::Context& ctx) {
  // create the loop's blocks
  auto guard_block = assembly::BasicBlock::labelled(guard_label());
  auto& guard_ref = *guard_block;
  guard_ref.comment() << "while#" << id_;
  auto body_block = assembly::BasicBlock::labelled(body_label());
  auto after_block = assembly::BasicBlock::labelled(end_label());

  // create conditional context for the guard
  control_flow::ConditionalContext cond_ctx{*body_block, *after_block};

  // generate the conditional guard
  ctx.program.insert(assembly::Position::Next, std::move(guard_block));
  guard_->conditional_context(cond_ctx);
  if (!guard_->generate_code(ctx)) return false;
  guard_->value().materialise(ctx, guard_->token_start().loc);
  const int index = ctx.program.current().size();
  if (!cond_ctx.handled && !cond_ctx.generate_branches(ctx, *guard_, guard_->value())) return false;
  ctx.program.update_line_origins(token_start().loc, index);

  // generate the body
  ctx.program.insert(assembly::Position::Next, std::move(body_block));
  if (!body_->generate_code(ctx)) return false;

  // add branch back to the guard (form a loop)
  ctx.program.current().add(assembly::create_branch(assembly::Arg::label(guard_ref)));
  ctx.program.current().back().origin(token_end().loc);
  ctx.program.current().back().comment() << "loop while#" << id_;

  // create the `after` block for when the loop exits
  ctx.program.insert(assembly::Position::Next, std::move(after_block));

  return true;
}

std::string lang::ast::WhileStatementNode::guard_label() const {
  return "whileguard_" + std::to_string(id_);
}

std::string lang::ast::WhileStatementNode::body_label() const {
  return "whilebody_" + std::to_string(id_);
}

std::string lang::ast::WhileStatementNode::end_label() const {
  return "endwhile_" + std::to_string(id_);
}
