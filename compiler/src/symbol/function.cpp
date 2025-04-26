#include "function.hpp"
#include "ast/function/function_base.hpp"
#include "types/function.hpp"

lang::symbol::Function::Function(lang::lexer::Token name, ast::FunctionBaseNode& node)
  : Symbol(std::move(name), Category::Function, node.type()), node_(node) {}

bool lang::symbol::Function::define(lang::Context& ctx) const {
  return node_.define(ctx);
}
