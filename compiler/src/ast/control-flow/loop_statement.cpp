#include "loop_statement.hpp"
#include "types/graph.hpp"
#include "message_helper.hpp"
#include "context.hpp"
#include "assembly/create.hpp"
#include "control_flow/loop_context.hpp"
#include "types/unit.hpp"

static unsigned int current_id = 0;

lang::ast::LoopStatementNode::LoopStatementNode(lexer::Token token, std::unique_ptr<Node> body)
  : Node(std::move(token)), body_(std::move(body)), id_(current_id++) {
  token_end(body_->token_end());
}

std::ostream& lang::ast::LoopStatementNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "loop ";
  body_->print_code(os, indent_level + 1);
  return os;
}

std::ostream& lang::ast::LoopStatementNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);

  os << std::endl;
  body_->print_tree(os, indent_level + 1);
  return os;
}

bool lang::ast::LoopStatementNode::always_returns() const {
  return body_->always_returns();
}

bool lang::ast::LoopStatementNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  return body_->collate_registry(messages, registry);
}

bool lang::ast::LoopStatementNode::process(lang::Context& ctx) {
  // push a new loop context
  ctx.loops.emplace(body_label(), end_label());

  // process our body
  if (!body_->process(ctx)) return false;

  // remove loop context
  ctx.loops.pop();

  // our body should not return anything (i.e., `()`)
  if (!body_->resolve(ctx)) return false;
  auto& value = body_->value();
  if (value.type() != type::unit) {
    std::unique_ptr<message::BasicMessage> msg = token_start().generate_message(message::Error);
    msg->get() << "type mismatch: a loop should evaluate to unit '()', but got ";
    value.type().print_code(msg->get());
    ctx.messages.add(std::move(msg));

    msg = std::make_unique<message::BasicMessage>(message::Note);
    msg->get() << "did you forget a semicolon at the end of an expression?";
    ctx.messages.add(std::move(msg));
    return false;
  }

  return true;
}

bool lang::ast::LoopStatementNode::generate_code(lang::Context& ctx) {
  // create the loop's blocks
  auto body_block = assembly::BasicBlock::labelled(body_label());
  auto& body_ref = *body_block;
  body_ref.comment() << "loop#" << id_;
  auto after_block = assembly::BasicBlock::labelled(end_label());

  // generate the body
  ctx.program.insert(assembly::Position::Next, std::move(body_block));
  if (!body_->generate_code(ctx)) return false;

  // add branch back to the body (form a loop)
  ctx.program.current().add(assembly::create_branch(assembly::Arg::label(body_ref)));
  ctx.program.current().back().comment() << "loop#" << id_;
  ctx.program.current().back().origin(token_end().loc);

  // create the `after` block for when the loop exits
  ctx.program.insert(assembly::Position::Next, std::move(after_block));

  return true;
}

std::string lang::ast::LoopStatementNode::body_label() const {
  return "loopbody_" + std::to_string(id_);
}

std::string lang::ast::LoopStatementNode::end_label() const {
  return "endloop_" + std::to_string(id_);
}
