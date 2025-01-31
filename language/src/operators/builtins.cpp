#include <stdexcept>
#include <cassert>
#include "info.hpp"
#include "builtin.hpp"
#include "context.hpp"
#include "ast/types/function.hpp"
#include "ast/types/int.hpp"
#include "assembly/create.hpp"
#include "ast/types/float.hpp"
#include "ast/types/bool.hpp"

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
  return ctx.reg_alloc_manager.resolve_ref(maybe_ref.value(), true);
}

// fetch LHS and RHS argument pair, enforcing at least one is in a register
// return <register argument, other argument>
// argument: cast arguments to this type?
static std::pair<uint8_t, std::unique_ptr<lang::assembly::Arg>> fetch_argument_pair(lang::Context& ctx, std::optional<constants::inst::datatype::dt> cast_to = std::nullopt) {
  // fetch references to lhs and rhs
  auto lhs_ref = ctx.reg_alloc_manager.get_recent(1);
  assert(lhs_ref.has_value());
  auto rhs_ref = ctx.reg_alloc_manager.get_recent(0);
  assert(rhs_ref.has_value());

  // first, ensure both side has the correct type?
  if (cast_to.has_value()) {
    lhs_ref = ctx.reg_alloc_manager.guarantee_datatype(lhs_ref.value(), cast_to.value());
    rhs_ref = ctx.reg_alloc_manager.guarantee_datatype(rhs_ref.value(), cast_to.value());
  }

  // enforce that one side is from a register
  if (lhs_ref->type != lang::memory::Ref::Register) {
    return {
        ctx.reg_alloc_manager.guarantee_register(lhs_ref.value()).offset,
        ctx.reg_alloc_manager.resolve_ref(rhs_ref.value(), true)
    };
  }

  if (rhs_ref->type != lang::memory::Ref::Register) {
    return {
        ctx.reg_alloc_manager.guarantee_register(rhs_ref.value()).offset,
        ctx.reg_alloc_manager.resolve_ref(lhs_ref.value(), true)
    };
  }

  // otherwise, both in a register
  // arbitrarily choose LHS to go first
  return {
      lhs_ref.value().offset,
      ctx.reg_alloc_manager.resolve_ref(rhs_ref.value(), true)
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
  // return which register the result is in
  static uint8_t generate_add(Context& ctx, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, datatype);
    ctx.program.current().add(assembly::create_add(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for subtraction
  static uint8_t generate_sub(Context& ctx, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, datatype);
    ctx.program.current().add(assembly::create_sub(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for multiplication
  static uint8_t generate_mul(Context& ctx, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, datatype);
    ctx.program.current().add(assembly::create_mul(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for multiplication
  static uint8_t generate_div(Context& ctx, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, datatype);
    ctx.program.current().add(assembly::create_div(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for <<
  static uint8_t generate_shl(Context& ctx) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx);
    ctx.program.current().add(assembly::create_shift_left(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for >>
  static uint8_t generate_shr(Context& ctx) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx);
    ctx.program.current().add(assembly::create_shift_right(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for modulus
  static uint8_t generate_mod(Context& ctx) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx);
    ctx.program.current().add(assembly::create_mod(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for bitwise AND
  static uint8_t generate_and(Context& ctx) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx);
    ctx.program.current().add(assembly::create_and(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for bitwise OR
  static uint8_t generate_or(Context& ctx) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx);
    ctx.program.current().add(assembly::create_or(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for bitwise XOR
  static uint8_t generate_xor(Context& ctx) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx);
    ctx.program.current().add(assembly::create_xor(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for a comparison
  static uint8_t generate_cmp(Context& ctx, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, datatype);
    ctx.program.current().add(assembly::create_comparison(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // generate a comparison, setting a Boolean result if equal to a test flag
  static uint8_t generate_cmp_bool(Context& ctx, constants::inst::datatype::dt datatype, constants::cmp::flag cmp) {
    uint8_t reg = generate_cmp(ctx, datatype);
    // zero-out the register
    ctx.program.current().add(assembly::create_zero(reg));
    // set to true if flag matches
    auto inst = assembly::create_load(reg, assembly::Arg::imm(1));
    inst->set_conditional(cmp);
    ctx.program.current().add(std::move(inst));
    return reg;
  }
}

namespace init_builtin {
  using namespace lang;
  using namespace lang::ops;
  using namespace ast::type;

  static void addition() {
    // operator+(int32, int32)
    store_operator(std::make_unique<BuiltinOperator>(
        "+",
        FunctionNode::create({int32, int32}, int32),
        [](Context& ctx) { generators::generate_add(ctx, constants::inst::datatype::s32); }
    ));

    // operator+(uint32, uint32)
    store_operator(std::make_unique<BuiltinOperator>(
        "+",
        FunctionNode::create({uint32, uint32}, uint32),
        [](Context& ctx) { generators::generate_add(ctx, constants::inst::datatype::u32); }
    ));

    // operator+(int64, int64)
    store_operator(std::make_unique<BuiltinOperator>(
        "+",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_add(ctx, constants::inst::datatype::s64); }
    ));

    // operator+(uint64, uint64)
    store_operator(std::make_unique<BuiltinOperator>(
        "+",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_add(ctx, constants::inst::datatype::u64); }
    ));

    // operator+(float, float)
    store_operator(std::make_unique<BuiltinOperator>(
        "+",
        FunctionNode::create({float32, float32}, float32),
        [](Context& ctx) { generators::generate_add(ctx, constants::inst::datatype::flt); }
    ));

    // operator+(double, double)
    store_operator(std::make_unique<BuiltinOperator>(
        "+",
        FunctionNode::create({float64, float64}, float64),
        [](Context& ctx) { generators::generate_add(ctx, constants::inst::datatype::dbl); }
    ));
  }

  static void subtraction() {
    // operator-(int32, int32)
    store_operator(std::make_unique<BuiltinOperator>(
        "-",
        FunctionNode::create({int32, int32}, int32),
        [](Context& ctx) { generators::generate_sub(ctx, constants::inst::datatype::s32); }
    ));

    // operator-(uint32, uint32)
    store_operator(std::make_unique<BuiltinOperator>(
        "-",
        FunctionNode::create({uint32, uint32}, uint32),
        [](Context& ctx) { generators::generate_sub(ctx, constants::inst::datatype::u32); }
    ));

    // operator-(int64, int64)
    store_operator(std::make_unique<BuiltinOperator>(
        "-",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_sub(ctx, constants::inst::datatype::s64); }
    ));

    // operator-(uint64, uint64)
    store_operator(std::make_unique<BuiltinOperator>(
        "-",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_sub(ctx, constants::inst::datatype::u64); }
    ));

    // operator-(float, float)
    store_operator(std::make_unique<BuiltinOperator>(
        "-",
        FunctionNode::create({float32, float32}, float32),
        [](Context& ctx) { generators::generate_sub(ctx, constants::inst::datatype::flt); }
    ));

    // operator-(double, double)
    store_operator(std::make_unique<BuiltinOperator>(
        "-",
        FunctionNode::create({float64, float64}, float64),
        [](Context& ctx) { generators::generate_sub(ctx, constants::inst::datatype::dbl); }
    ));
  }

  static void multiplication() {
    // operator*(int32, int32)
    store_operator(std::make_unique<BuiltinOperator>(
        "*",
        FunctionNode::create({int32, int32}, int32),
        [](Context& ctx) { generators::generate_mul(ctx, constants::inst::datatype::s32); }
    ));

    // operator*(uint32, uint32)
    store_operator(std::make_unique<BuiltinOperator>(
        "*",
        FunctionNode::create({uint32, uint32}, uint32),
        [](Context& ctx) { generators::generate_mul(ctx, constants::inst::datatype::u32); }
    ));

    // operator*(int64, int64)
    store_operator(std::make_unique<BuiltinOperator>(
        "*",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_mul(ctx, constants::inst::datatype::s64); }
    ));

    // operator*(uint64, uint64)
    store_operator(std::make_unique<BuiltinOperator>(
        "*",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_mul(ctx, constants::inst::datatype::u64); }
    ));

    // operator*(float, float)
    store_operator(std::make_unique<BuiltinOperator>(
        "*",
        FunctionNode::create({float32, float32}, float32),
        [](Context& ctx) { generators::generate_mul(ctx, constants::inst::datatype::flt); }
    ));

    // operator*(double, double)
    store_operator(std::make_unique<BuiltinOperator>(
        "*",
        FunctionNode::create({float64, float64}, float64),
        [](Context& ctx) { generators::generate_mul(ctx, constants::inst::datatype::dbl); }
    ));
  }

  static void division() {
    // operator/(int32, int32)
    store_operator(std::make_unique<BuiltinOperator>(
        "/",
        FunctionNode::create({int32, int32}, int32),
        [](Context& ctx) { generators::generate_div(ctx, constants::inst::datatype::s32); }
    ));

    // operator/(uint32, uint32)
    store_operator(std::make_unique<BuiltinOperator>(
        "/",
        FunctionNode::create({uint32, uint32}, uint32),
        [](Context& ctx) { generators::generate_div(ctx, constants::inst::datatype::u32); }
    ));

    // operator/(int64, int64)
    store_operator(std::make_unique<BuiltinOperator>(
        "/",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_div(ctx, constants::inst::datatype::s64); }
    ));

    // operator/(uint64, uint64)
    store_operator(std::make_unique<BuiltinOperator>(
        "/",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_div(ctx, constants::inst::datatype::u64); }
    ));

    // operator/(float, float)
    store_operator(std::make_unique<BuiltinOperator>(
        "/",
        FunctionNode::create({float32, float32}, float32),
        [](Context& ctx) { generators::generate_div(ctx, constants::inst::datatype::flt); }
    ));

    // operator/(double, double)
    store_operator(std::make_unique<BuiltinOperator>(
        "/",
        FunctionNode::create({float64, float64}, float64),
        [](Context& ctx) { generators::generate_div(ctx, constants::inst::datatype::dbl); }
    ));
  }

  static void shift() {
    // operator<<(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "<<",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_shl(ctx); }
    ));

    // operator<<(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "<<",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_shl(ctx); }
    ));

    // operator>>(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        ">>",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_shr(ctx); }
    ));

    // operator>>(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        ">>",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_shr(ctx); }
    ));
  }

  static void modulo() {
    // operator%(u64, i32)
    store_operator(std::make_unique<BuiltinOperator>(
        "%",
        FunctionNode::create({uint64, int32}, int64),
        [](Context& ctx) { generators::generate_mod(ctx); }
    ));

    // TODO operator%(i64, i32)
  }

  static void bitwise_and() {
    // operator&(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "&",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_and(ctx); }
    ));

    // operator&(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "&",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_and(ctx); }
    ));
  }

  static void bitwise_or() {
    // operator|(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "|",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_or(ctx); }
    ));

    // operator|(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "|",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_or(ctx); }
    ));
  }

  static void bitwise_xor() {
    // operator^(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "^",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx) { generators::generate_xor(ctx); }
    ));

    // operator^(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "^",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx) { generators::generate_xor(ctx); }
    ));
  }

  static void relational() {
    static const std::unordered_map<std::string, constants::cmp::flag> ops = {
        {"==", constants::cmp::eq},
        {"!=", constants::cmp::ne},
        {"<", constants::cmp::lt},
        {"<=", constants::cmp::le},
        {">", constants::cmp::gt},
        {">=", constants::cmp::ge},
    };

    for (const auto& [op, cmp] : ops) {
      for (const Node& type : numerical) {
        const auto asm_type = type.get_asm_datatype();

        // operatorX(T, T)
        store_operator(std::make_unique<BuiltinOperator>(
            op,
            FunctionNode::create({type, type}, boolean),
            [cmp, asm_type](Context& ctx) { generators::generate_cmp_bool(ctx, asm_type, cmp); }
        ));
      }
    }

    // operator!=(bool, bool)
    store_operator(std::make_unique<BuiltinOperator>(
        "!=",
        FunctionNode::create({boolean, boolean}, boolean),
        [](Context& ctx) { generators::generate_xor(ctx); }
    ));

    // operator==(bool, bool)
    store_operator(std::make_unique<BuiltinOperator>(
        "==",
        FunctionNode::create({boolean, boolean}, boolean),
        [](Context& ctx) {
          uint8_t reg = generators::generate_xor(ctx);
          ctx.program.current().add(assembly::create_not(reg, reg));
        }
    ));
  }
}

void lang::ops::init_builtins() {
  using namespace init_builtin;
  addition();
  subtraction();
  multiplication();
  division();
  shift();
  modulo();
  bitwise_and();
  bitwise_or();
  bitwise_xor();
  relational();
}

bool lang::ops::implicit_cast(lang::Context& ctx, constants::inst::datatype::dt target) {
  // get most recent location
  auto maybe_ref = ctx.reg_alloc_manager.get_recent();
  assert(maybe_ref.has_value());
  auto ref = maybe_ref.value();

  // if types already match, exit
  if (ctx.reg_alloc_manager.find(ref).datatype.get_asm_datatype() == target) {
    return false;
  }

  // otherwise, convert!
  ctx.reg_alloc_manager.guarantee_datatype(ref, target);
  return true;
}
