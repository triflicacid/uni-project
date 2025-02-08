#include <stdexcept>
#include <cassert>
#include "info.hpp"
#include "builtin.hpp"
#include "context.hpp"
#include "assembly/basic_block.hpp"
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

using Args = const std::deque<std::reference_wrapper<const lang::value::Value>>&;

// fetch the given argument, return assembly argument to resolve it
static std::unique_ptr<lang::assembly::Arg> fetch(lang::Context& ctx, Args args, ArgSelect arg_select) {
  // determine argument offset based on selection
  auto& value = arg_select == ArgSelect::rhs
      ? args[1]
      : args[0];
  const lang::memory::Ref& ref = value.get().rvalue().ref();
  return ctx.reg_alloc_manager.resolve_ref(ref, true);
}

// fetch the given argument, return assembly argument to resolve it
// returns the register location, as we guarantee register placement
// optionally, also guarantee the datatype
static uint8_t fetch_reg(lang::Context& ctx, Args args, ArgSelect arg_select, std::optional<constants::inst::datatype::dt> cast_to = std::nullopt) {
  // determine argument offset based on selection
  auto& value = arg_select == ArgSelect::rhs
                ? args[1]
                : args[0];
  lang::memory::Ref ref = value.get().rvalue().ref();

  if (cast_to.has_value()) ref = ctx.reg_alloc_manager.guarantee_datatype(ref, cast_to.value());
  else ref = ctx.reg_alloc_manager.guarantee_register(ref);
  return ref.offset;
}

// fetch LHS and RHS argument pair, enforcing at least one is in a register
// return <register argument, other argument>
// argument: cast arguments to this type?
static std::pair<uint8_t, std::unique_ptr<lang::assembly::Arg>> fetch_argument_pair(lang::Context& ctx, Args args, std::optional<constants::inst::datatype::dt> cast_to = std::nullopt) {
  // fetch references to lhs and rhs
  const auto& lhs = args[0].get();
  lang::memory::Ref lhs_ref = lhs.rvalue().ref();
  const auto& rhs = args[1].get();
  lang::memory::Ref rhs_ref = rhs.rvalue().ref();

  // first, ensure both side has the correct type?
  if (cast_to.has_value()) {
    lhs_ref = ctx.reg_alloc_manager.guarantee_datatype(lhs_ref, cast_to.value());
    rhs_ref = ctx.reg_alloc_manager.guarantee_datatype(rhs_ref, cast_to.value());
  }

  // enforce that one side is from a register
  if (lhs_ref.type != lang::memory::Ref::Register) {
    return {
        ctx.reg_alloc_manager.guarantee_register(lhs_ref).offset,
        ctx.reg_alloc_manager.resolve_ref(rhs_ref, true)
    };
  }

  if (rhs_ref.type != lang::memory::Ref::Register) {
    return {
        ctx.reg_alloc_manager.guarantee_register(rhs_ref).offset,
        ctx.reg_alloc_manager.resolve_ref(lhs_ref, true)
    };
  }

  // otherwise, both in a register
  // arbitrarily choose LHS to go first
  return {
      lhs_ref.offset,
      ctx.reg_alloc_manager.resolve_ref(rhs_ref, true)
  };
}

std::unordered_map<std::string, const lang::ops::OperatorInfo> lang::ops::builtin_binary{
    {"=", {1, true, false}},    // assignment

    {"||", {7, false, false}},  // logical OR
    {"&&", {8, false, false}},  // logical AND
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
    {"as", {17, false, false}},  // cast
    {"()", {19, false}},  // function call
    {".", {20, false, false}},  // member access
};

std::unordered_map<std::string, const lang::ops::OperatorInfo> lang::ops::builtin_unary{
    {"-", {18, true}}, // negation
    {"!", {18, true}}, // logical NOT
    {"~", {18, true}}, // bitwise NOT
    {"(type)", {18, true, false}}, // primitive cast
    {"registerof", {18, true, false}}, // register lookup
    {"register", {18, true, false}}, // get register
};

const lang::ops::OperatorInfo lang::ops::generic_binary{2, false};
const lang::ops::OperatorInfo lang::ops::generic_unary{14, true};

namespace generators {
  using namespace lang;

  // generate an addition instruction for the given asm datatype
  // assume values stored in LHS and RHS are compatible with the asm datatype
  // return which register the result is in
  static uint8_t generate_add(Context& ctx, Args args, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_add(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for subtraction
  static uint8_t generate_sub(Context& ctx, Args args, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_sub(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for multiplication
  static uint8_t generate_mul(Context& ctx, Args args, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_mul(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for multiplication
  static uint8_t generate_div(Context& ctx, Args args, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_div(datatype, reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for <<
  static uint8_t generate_shl(Context& ctx, Args args) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args);
    ctx.program.current().add(assembly::create_shift_left(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for >>
  static uint8_t generate_shr(Context& ctx, Args args) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args);
    ctx.program.current().add(assembly::create_shift_right(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for modulus
  static uint8_t generate_mod(Context& ctx, Args args) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args);
    ctx.program.current().add(assembly::create_mod(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for bitwise AND
  static uint8_t generate_and(Context& ctx, Args args) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args);
    ctx.program.current().add(assembly::create_and(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for bitwise OR
  static uint8_t generate_or(Context& ctx, Args args) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args);
    ctx.program.current().add(assembly::create_or(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for bitwise XOR
  static uint8_t generate_xor(Context& ctx, Args args) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args);
    ctx.program.current().add(assembly::create_xor(reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for bitwise NOT
  static uint8_t generate_bitwise_not(Context& ctx, Args args) {
    uint8_t reg = fetch_reg(ctx, args, ArgSelect::sole);
    ctx.program.current().add(assembly::create_not(reg, reg));
    return reg;
  }

  // like generate_add, but for Boolean NOT
  static uint8_t generate_boolean_not(Context& ctx, Args args) {
    uint8_t reg = fetch_reg(ctx, args, ArgSelect::sole);
    ctx.program.current().add(assembly::create_xor(reg, reg, assembly::Arg::imm(1)));
    return reg;
  }

  // like generate_add, but for a comparison
  static uint8_t generate_cmp(Context& ctx, Args args, constants::inst::datatype::dt datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_comparison(datatype, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // generate a comparison, setting a Boolean result if equal to a test flag
  static uint8_t generate_cmp_bool(Context& ctx, Args args, constants::inst::datatype::dt datatype, constants::cmp::flag cmp) {
    uint8_t reg = generate_cmp(ctx, args, datatype);
    // zero-out the register
    ctx.program.current().add(assembly::create_zero(reg));
    // set to true if flag matches
    ctx.program.current().add(set_conditional(assembly::create_load(reg, assembly::Arg::imm(1)), cmp));
    return reg;
  }

  // like generate_add, but for negation
  static uint8_t generate_neg(Context& ctx, Args args, constants::inst::datatype::dt datatype) {
    uint8_t reg_arg = fetch_reg(ctx, args, ArgSelect::sole, datatype);

    // load 0 into register
    // TODO make more efficient?
    // TODO how can we ensure both are in a register?
    const memory::Literal& zero = memory::Literal::zero(ast::type::from_asm_type(datatype));
    memory::Ref ref = ctx.reg_alloc_manager.guarantee_register(ctx.reg_alloc_manager.find_or_insert(zero));

    ctx.program.current().add(assembly::create_sub(datatype, reg_arg, ref.offset, assembly::Arg::reg(reg_arg)));
    return reg_arg;
  }
}

namespace init_builtin {
  using namespace lang;
  using namespace lang::ops;
  using namespace ast::type;

  static void addition() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      const auto asm_type = type.get().get_asm_datatype();

      // operator+(T, T)
      store_operator(std::make_unique<BuiltinOperator>(
          "+",
          FunctionNode::create({type, type}, type),
          [asm_type](Context& ctx, Args args) { return generators::generate_add(ctx, args, asm_type); }
      ));
    }
  }

  static void subtraction() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      const auto asm_type = type.get().get_asm_datatype();

      // operator-(T, T)
      store_operator(std::make_unique<BuiltinOperator>(
          "-",
          FunctionNode::create({type, type}, type),
          [asm_type](Context& ctx, Args args) { return generators::generate_sub(ctx, args, asm_type); }
      ));
    }
  }

  static void multiplication() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      const auto asm_type = type.get().get_asm_datatype();

      // operator*(T, T)
      store_operator(std::make_unique<BuiltinOperator>(
          "*",
          FunctionNode::create({type, type}, type),
          [asm_type](Context& ctx, Args args) { return generators::generate_mul(ctx, args, asm_type); }
      ));
    }
  }

  static void division() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      const auto asm_type = type.get().get_asm_datatype();

      // operator/(T, T)
      store_operator(std::make_unique<BuiltinOperator>(
          "/",
          FunctionNode::create({type, type}, type),
          [asm_type](Context& ctx, Args args) { return generators::generate_div(ctx, args, asm_type); }
      ));
    }
  }

  static void shift() {
    // operator<<(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "<<",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx, Args args) { return generators::generate_shl(ctx, args); }
    ));

    // operator<<(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "<<",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx, Args args) { return generators::generate_shl(ctx, args); }
    ));

    // operator>>(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        ">>",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx, Args args) { return generators::generate_shr(ctx, args); }
    ));

    // operator>>(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        ">>",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx, Args args) { return generators::generate_shr(ctx, args); }
    ));
  }

  static void modulo() {
    // operator%(u64, i32)
    store_operator(std::make_unique<BuiltinOperator>(
        "%",
        FunctionNode::create({uint64, int32}, int64),
        [](Context& ctx, Args args) { return generators::generate_mod(ctx, args); }
    ));

    // TODO operator%(i64, i32)
  }

  static void bitwise_and() {
    // operator&(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "&",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx, Args args) { return generators::generate_and(ctx, args); }
    ));

    // operator&(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "&",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx, Args args) { return generators::generate_and(ctx, args); }
    ));
  }

  static void bitwise_or() {
    // operator|(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "|",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx, Args args) { return generators::generate_or(ctx, args); }
    ));

    // operator|(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "|",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx, Args args) { return generators::generate_or(ctx, args); }
    ));
  }

  static void bitwise_xor() {
    // operator^(u64, u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "^",
        FunctionNode::create({uint64, uint64}, uint64),
        [](Context& ctx, Args args) { return generators::generate_xor(ctx, args); }
    ));

    // operator^(i64, i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "^",
        FunctionNode::create({int64, int64}, int64),
        [](Context& ctx, Args args) { return generators::generate_xor(ctx, args); }
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
            [cmp, asm_type](Context& ctx, Args args) { return generators::generate_cmp_bool(ctx, args, asm_type, cmp); }
        ));
      }
    }

    // operator!=(bool, bool)
    store_operator(std::make_unique<BuiltinOperator>(
        "!=",
        FunctionNode::create({boolean, boolean}, boolean),
        [](Context& ctx, Args args) { return generators::generate_xor(ctx, args); }
    ));

    // operator==(bool, bool)
    store_operator(std::make_unique<BuiltinOperator>(
        "==",
        FunctionNode::create({boolean, boolean}, boolean),
        [](Context& ctx, Args args) {
          uint8_t reg = generators::generate_xor(ctx, args);
          ctx.program.current().add(assembly::create_not(reg, reg));
          return reg;
        }
    ));
  }

  static void bitwise_not() {
    // operator~(u64)
    store_operator(std::make_unique<BuiltinOperator>(
        "~",
        FunctionNode::create({uint64}, uint64),
        [](Context& ctx, Args args) { return generators::generate_bitwise_not(ctx, args); }
    ));

    // operator~(i64)
    store_operator(std::make_unique<BuiltinOperator>(
        "~",
        FunctionNode::create({int64}, int64),
        [](Context& ctx, Args args) { return generators::generate_bitwise_not(ctx, args); }
    ));
  }

  static void boolean_not() {
    // operator!(bool)
    store_operator(std::make_unique<BuiltinOperator>(
        "!",
        FunctionNode::create({boolean}, boolean),
        [](Context& ctx, Args args) { return generators::generate_boolean_not(ctx, args); }
    ));
  }

  static void negation() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      const auto asm_type = type.get().get_asm_datatype();

      // operator-(T)
      store_operator(std::make_unique<BuiltinOperator>(
          "-",
          FunctionNode::create({type}, type),
          [asm_type](Context& ctx, Args args) { return generators::generate_neg(ctx, args, asm_type); }
      ));
    }
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
  bitwise_not();
  boolean_not();
  relational();
  negation();
}

lang::memory::Ref lang::ops::cast(lang::Context& ctx, const ast::type::Node& target) {
  // get most recent location
  auto maybe_ref = ctx.reg_alloc_manager.get_recent();
  assert(maybe_ref.has_value());
  const memory::Ref ref = maybe_ref.value();

  // if types already match, exit
  if (ctx.reg_alloc_manager.find(ref).value->type() == target) {
    return ref;
  }

  // otherwise, convert!
  return ctx.reg_alloc_manager.guarantee_datatype(ref, target);
}

void lang::ops::generate_bool_cast(assembly::BasicBlock& block, uint8_t reg) {
  // compare with 0
  block.add(assembly::create_comparison(reg, assembly::Arg::imm(0x0)));
  auto& comment = block.back().comment();
  // zero out the register if ==
  block.add(set_conditional(assembly::create_zero(reg), constants::cmp::eq));
  // set to 1 if !=
  block.add(set_conditional(assembly::create_load(reg, assembly::Arg::imm(1)), constants::cmp::ne));
  // add comment
  comment << "boolean cast";
}
