#include "if_else.hpp"

lang::ast::IfElseNode::IfElseNode(lang::lexer::Token token, std::unique_ptr<Node> guard, std::unique_ptr<Node> then_body,
                                  std::optional<std::unique_ptr<Node>> else_body)
    : Node(std::move(token)), guard_(std::move(guard)), then_(std::move(then_body)), else_(std::move(else_body)) {
  token_end(else_.has_value() ? else_.value()->token_end() : then_->token_end());
}

std::ostream& lang::ast::IfElseNode::print_code(std::ostream& os, unsigned int indent_level) const {
  os << "if ";
  guard_->print_code(os, indent_level) << " ";
  then_->print_code(os, indent_level + 1);

  if (else_.has_value()) {
    os << std::endl;
    indent(os, indent_level) << "else ";
    else_.value()->print_code(os, indent_level + 1);
  }

  return os;
}

std::ostream& lang::ast::IfElseNode::print_tree(std::ostream& os, unsigned int indent_level) const {
  Node::print_tree(os, indent_level) << std::endl;
  guard_->print_tree(os, indent_level + 1) << std::endl;
  then_->print_tree(os, indent_level + 1);

  if (else_.has_value()) {
    os << std::endl;
    else_.value()->print_tree(os, indent_level + 1);
  }

  return os;
}

bool lang::ast::IfElseNode::always_returns() const {
  return then_->always_returns() && (!else_.has_value() || else_.value()->always_returns());
}

bool lang::ast::IfElseNode::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  return guard_->collate_registry(messages, registry)
    && then_->collate_registry(messages, registry)
    && (!else_.has_value() || else_.value()->collate_registry(messages, registry));
}

const lang::value::Value& lang::ast::IfElseNode::value() const {
  if (result_.has_value()) return result_->get();
  return Node::value();
}
