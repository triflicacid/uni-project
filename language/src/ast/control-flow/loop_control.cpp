#include "loop_control.hpp"
#include "context.hpp"
#include "assembly/create.hpp"

lang::ast::LoopControlNode::LoopControlNode(lang::lexer::Token token) : Node(token) {
  if (token.type == lexer::TokenType::break_kw) {
    variant_ = Variant::Break;
  } else {
    variant_ = Variant::Continue;
  }
}

std::string lang::ast::LoopControlNode::node_name() const {
  return token_start().image;
}

std::ostream& lang::ast::LoopControlNode::print_code(std::ostream& os, unsigned int indent_level) const {
  return os << node_name();
}

bool lang::ast::LoopControlNode::process(lang::Context& ctx) {
  // check that we are in a loop
  if (ctx.loops.empty()) {
    auto msg = generate_message(message::Error);
    msg->get() << "'" << node_name() << "' statement must be inside a loop";
    ctx.messages.add(std::move(msg));
    return false;
  }

  // otherwise, set loop_
  loop_ = ctx.loops.top();

  return true;
}

bool lang::ast::LoopControlNode::generate_code(lang::Context& ctx) {
  assert(loop_.has_value());

  // jump to start or end depending on variant
  const std::string& label = variant_ == Variant::Break
      ? loop_->get().end
      : loop_->get().start;

  // jump to this label
  ctx.program.current().add(assembly::create_branch(
      assembly::Arg::label(label)
    ));
  ctx.program.current().back().comment() << node_name();

  return true;
}
