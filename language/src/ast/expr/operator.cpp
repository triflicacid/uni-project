#include <cassert>
#include "operator.hpp"
#include "shell.hpp"
#include "ast/types/function.hpp"
#include "operators/operator.hpp"
#include "context.hpp"
#include "operators/builtin.hpp"

const lang::ast::type::Node &lang::ast::expr::OperatorNode::type() const {
  assert(op_.has_value());
  return op_.value().get().type().returns();
}

bool lang::ast::expr::OperatorNode::process(lang::Context& ctx) {
  // generate our signature
  auto& signature = this->signature();

  // search for a matching operator
  auto operators = ops::get(token_.image);

  // extract types into an option list
  std::deque<std::reference_wrapper<const type::FunctionNode>> candidates;
  for (auto& op : operators) {
    candidates.push_back(op.get().type());
  }

  // filter to viable candidates
  candidates = signature.filter_candidates(candidates);

  // if only one candidate remaining: this is us! use signature + name to find operator object
  if (candidates.size() == 1) {
    op_ = ops::get(token_.image, candidates.front());
    return true;
  }

  // otherwise, if candidates is non-empty, we can narrow options down for message
  if (!candidates.empty()) {
    operators.clear();
    for (auto& candidate : candidates) {
      operators.push_back(ops::get(token_.image, candidate).value());
    }
  }

  // error to user, reporting our candidate list
  std::unique_ptr<message::BasicMessage> msg = token_.generate_message(message::Error);
  msg->get() << "unable to resolve a suitable candidate for operator" << symbol();
  signature.print_code(msg->get());
  ctx.messages.add(std::move(msg));

  for (auto& option : operators) {
    msg = std::make_unique<message::BasicMessage>(message::Note);
    msg->get() << "candidate: operator" << option.get().symbol();
    option.get().type().print_code(msg->get());
    ctx.messages.add(std::move(msg));
  }
  return false;
}

bool lang::ast::expr::OperatorNode::load(lang::Context& ctx) const {
  // operator must have been set by ::process
  assert(op_.has_value());

  // TODO user-defined operators
  auto& op = op_.value().get();
  assert(op.builtin());

  // generate asm code and mark with comment
  auto& builtin = static_cast<const ops::BuiltinOperator&>(op);
  builtin.process(ctx);
  auto& comment = ctx.program.current().back().comment();
  comment << "operator" << op_.value().get().symbol();
  op_.value().get().type().print_code(comment);

  return true;
}

std::ostream &lang::ast::expr::BinaryOperatorNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "(";
  lhs_->print_code(os, indent_level);
  os << " " << token_.image << " ";
  rhs_->print_code(os, indent_level);
  return os << ")";
}

std::ostream &lang::ast::expr::BinaryOperatorNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << token_.image << SHELL_RESET << std::endl;

  lhs_->print_tree(os, indent_level + 1) << std::endl;
  rhs_->print_tree(os, indent_level + 1);
  return os;
}

const lang::ast::type::FunctionNode& lang::ast::expr::BinaryOperatorNode::signature() {
  // do not give a return type, it is not requires for selecting a candidate
  return type::FunctionNode::create({
    lhs_->type(),
    rhs_->type()
  },  std::nullopt);
}

bool lang::ast::expr::BinaryOperatorNode::process(lang::Context& ctx) {
  return lhs_->process(ctx) && rhs_->process(ctx) && OperatorNode::process(ctx);
}

bool lang::ast::expr::BinaryOperatorNode::load(lang::Context& ctx) const {
  // load arguments and invoke operator
  return lhs_->load(ctx) && rhs_->load(ctx) && OperatorNode::load(ctx);
}

std::ostream &lang::ast::expr::UnaryOperatorNode::print_code(std::ostream &os, unsigned int indent_level) const {
  os << "(" << token_.image << " ";
  expr_->print_code(os, indent_level);
  return os << ")";
}

std::ostream &lang::ast::expr::UnaryOperatorNode::print_tree(std::ostream &os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << token_.image << SHELL_RESET << std::endl;

  expr_->print_tree(os, indent_level + 1);
  return os;
}

const lang::ast::type::FunctionNode& lang::ast::expr::UnaryOperatorNode::signature() {
  const type::Node& expr_type = expr_->type();
  return type::FunctionNode::create({expr_type}, std::nullopt);
}

bool lang::ast::expr::UnaryOperatorNode::process(Context& ctx) {
  return expr_->process(ctx) && OperatorNode::process(ctx);
}

bool lang::ast::expr::UnaryOperatorNode::load(lang::Context& ctx) const {
  // load argument and invoke operator
  return expr_->load(ctx) && OperatorNode::load(ctx);
}

std::ostream& lang::ast::expr::CastOperatorNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "(";
  target_.print_code(os);
  os << ") ";
  expr_->print_code(os, indent_level);
  return os;
}

std::ostream& lang::ast::expr::CastOperatorNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level);
  os << SHELL_GREEN << "(";
  target_.print_code(os);
  os << ")" SHELL_RESET << std::endl;

  expr_->print_tree(os, indent_level + 1);
  return os;
}

lang::ast::expr::CastOperatorNode::CastOperatorNode(lang::lexer::Token token, const lang::ast::type::Node& target, std::unique_ptr<Node> expr)
  : UnaryOperatorNode(std::move(token), std::move(expr)), target_(target) {
  std::stringstream stream;
  stream << '(';
  target_.print_code(stream);
  stream << ')';
  symbol_ = stream.str();
}

bool lang::ast::expr::CastOperatorNode::process(lang::Context& ctx) {
  // this operator is special in that it doesn't look for operator overloads
  // just use the cvt<x>2<y> instruction
  return expr_->process(ctx);
}

bool lang::ast::expr::CastOperatorNode::load(lang::Context& ctx) const {
  if (!expr_->load(ctx)) return false;

  // emit conversion instruction
  ops::implicit_cast(ctx, target_.get_asm_datatype());
  return true;
}

const lang::ast::type::Node& lang::ast::expr::CastOperatorNode::type() const {
  return target_;
}
