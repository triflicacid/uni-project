#include "node.hpp"
#include "shell.hpp"
#include "context.hpp"
#include "ast/types/unit.hpp"

std::ostream &lang::ast::indent(std::ostream &os, unsigned int level) {
  return os << std::string(level * 2, ' ');
}

const lang::value::Value& lang::ast::Node::value() const {
  return value_
    ? *value_
    : value::unit_value;
}

std::ostream &lang::ast::Node::print_tree(std::ostream &os, unsigned int indent_level) const {
  indent(os, indent_level);
  const lexer::Token& t = token_start();
  return os << SHELL_LIGHT_PURPLE << node_name() << SHELL_RESET " @<" SHELL_BROWN
            << t.loc.path() << ":" << t.loc.line() << ":" << t.loc.column() << SHELL_RESET "> ";
}

bool lang::ast::Node::process(Context& ctx) {
  throw std::runtime_error(node_name() + "::process is unimplemented");
}

bool lang::ast::Node::resolve_value(Context& ctx) {
  if (value().is_future_lvalue() && !resolve_lvalue(ctx)) return false;
  if (value().is_future_rvalue() && !resolve_rvalue(ctx)) return false;
  return true;
}
