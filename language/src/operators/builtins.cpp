#include <stdexcept>
#include <cassert>
#include "info.hpp"
#include "builtin.hpp"
#include "context.hpp"
#include "ast/types/function.hpp"
#include "ast/types/int.hpp"
#include "assembly/create.hpp"

enum class ArgSelect {
  lhs, // lhs argument in binary operator
  rhs, // lhs argument in binary operator
  sole // sole argument in unary operator
};

// fetch the given argument, return assembly argument to resolve it
// second arg: guarantee that the value is in a register (default is false)
static std::unique_ptr<lang::assembly::Arg> fetch(lang::Context& ctx, ArgSelect arg_select, bool guarantee_reg = false) {
  // determine argument offset based on selection
  int offset = arg_select == ArgSelect::lhs
      ? 1
      : 0;
  auto maybe_ref = ctx.reg_alloc_manager.get_recent(offset);
  assert(maybe_ref.has_value());
  // guaranteed register placement?
  if (guarantee_reg) ctx.reg_alloc_manager.guarantee_register(maybe_ref.value());
  return ctx.reg_alloc_manager.resolve_ref(maybe_ref.value());
}

// fetch LHS and RHS argument pair, enforcing at least one is in a register
// return <register argument, other argument>
static std::pair<uint8_t, std::unique_ptr<lang::assembly::Arg>> fetch_argument_pair(lang::Context& ctx) {
  // fetch references to lhs and rhs
  const auto lhs_ref = ctx.reg_alloc_manager.get_recent(1);
  assert(lhs_ref.has_value());
  const auto rhs_ref = ctx.reg_alloc_manager.get_recent(0);
  assert(rhs_ref.has_value());

  // enforce that one side is from a register
  if (lhs_ref->type != lang::memory::Ref::Register) {
    return {
      ctx.reg_alloc_manager.guarantee_register(lhs_ref.value()).offset,
      ctx.reg_alloc_manager.resolve_ref(rhs_ref.value())
    };
  }

  if (rhs_ref->type != lang::memory::Ref::Register) {
    return {
        ctx.reg_alloc_manager.guarantee_register(rhs_ref.value()).offset,
        ctx.reg_alloc_manager.resolve_ref(lhs_ref.value())
    };
  }

  // otherwise, both in a register
  // arbitrarily choose LHS to go first
  return {
      rhs_ref.value().offset,
      ctx.reg_alloc_manager.resolve_ref(rhs_ref.value())
  };
}

std::unordered_map<std::string, const lang::ops::OperatorInfo> lang::ops::builtin_binary{
    {"=", {1, true}},    // assignment

    {"||", {7, false}},  // logical OR
    {"&&", {8, false}},  // logical AND
    {"|", {9, false}},   // bitwise OR
    {"^", {10, false}},  // bitwise XOR
    {"&", {11, false}},  // bitwise AND
    {"==", {12, false}}, // is equal to
    {"!=", {12, false}}, // is not equal to
    {"<", {12, false}},  // is less than
    {"<=", {12, false}}, // is less than or equal to
    {">", {12, false}},  // is greater than
    {">=", {12, false}}, // is greater than or equal to
    {">>", {13, false}}, // bitwise shift right
    {"<<", {13, false}}, // bitwise shift left
    {"+", {14, false}},  // addition
    {"-", {14, false}},  // subtraction
    {"*", {15, false}},  // multiplication
    {"/", {15, false}},  // division
    {"%", {15, false}},  // modulus
    {".", {20, false}},  // member access
};

std::unordered_map<std::string, const lang::ops::OperatorInfo> lang::ops::builtin_unary{
    {"-", {14, true}}, // negation
    {"!", {14, true}}, // logical NOT
    {"~", {14, true}}, // bitwise NOT
};

const lang::ops::OperatorInfo lang::ops::generic{2, false};

void lang::ops::BuiltinOperator::process(lang::Context& ctx) const {
  generator_(ctx);
}

namespace generators {
  using namespace lang;

  // generate an addition instruction for the given asm datatype
  // assume values stored in LHS and RHS are compatible with the asm datatype
  static void generate_add(Context& ctx, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx);
    ctx.program.current().add(assembly::create_add(datatype, reg_arg, reg_arg, std::move(other_arg)));
  }
}

void lang::ops::init_builtins() {
  using namespace ast::type;

  // TODO implement some built-in operators
  // operator+(int32, int32)
  store_operator(std::make_unique<BuiltinOperator>(
      "+",
      FunctionNode::create({int32, int32}, int32),
      [](Context& ctx) { generators::generate_add(ctx, constants::inst::datatype::s32); }
    ));
}
