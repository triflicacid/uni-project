#include "node.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "message_helper.hpp"

std::ostream &lang::ast::indent(std::ostream &os, unsigned int level) {
  return os << std::string(level * 2, ' ');
}

optional_ref<const lang::value::Value> lang::ast::process_node_and_expect(Node& node, Context& ctx, bool expect_lvalue) {
  if (!node.process(ctx) || node.resolve(ctx)) return std::nullopt;

  // extract value and check if correct category
  auto& value = node.value();
  if ((expect_lvalue && !value.is_lvalue()) || !value.is_rvalue()) {
    ctx.messages.add(util::error_expected_lrvalue(node, value.type(), expect_lvalue));
    return std::nullopt;
  }

  return value;
}

const lang::value::Value& lang::ast::Node::value() const {
  return value_
    ? *value_
    : *value::unit_value_instance;
}

std::ostream &lang::ast::Node::print_tree(std::ostream &os, unsigned int indent_level) const {
  indent(os, indent_level);
  const lexer::Token& t = token_start();
  return os << SHELL_LIGHT_PURPLE << node_name() << SHELL_RESET " @<" SHELL_BROWN
            << t.loc.path() << ":" << t.loc.line() << ":" << t.loc.column() << SHELL_RESET "> ";
}

bool lang::ast::Node::collate_registry(message::List& messages, lang::symbol::Registry& registry) {
  return true;
}

bool lang::ast::Node::generate_code(lang::Context& ctx) const {
  return true;
}
