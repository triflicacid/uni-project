#include <stdexcept>
#include "info.hpp"
#include "builtins.hpp"
#include "builtin.hpp"
#include "context.hpp"
#include "assembly/basic_block.hpp"
#include "ast/function/function_base.hpp"
#include "ast/types/function.hpp"
#include "ast/types/int.hpp"
#include "ast/types/float.hpp"
#include "ast/types/bool.hpp"
#include "ast/types/pointer.hpp"
#include "assembly/create.hpp"
#include "message_helper.hpp"

using Args = const std::deque<std::reference_wrapper<const lang::value::Value>>&;

// fetch the given argument, return assembly argument to resolve it
static std::unique_ptr<lang::assembly::Arg> fetch(lang::Context& ctx, Args args, int arg) {
  // determine argument offset based on selection
  auto& value = args[arg];
  const lang::memory::Ref& ref = value.get().rvalue().ref();
  return ctx.reg_alloc_manager.resolve_ref(ref, true);
}

// fetch the given argument, return assembly argument to resolve it
// returns the register location, as we guarantee register placement
// optionally, also guarantee the datatype
static uint8_t fetch_reg(lang::Context& ctx, Args args, int arg_select, optional_ref<const lang::ast::type::Node> cast_to = std::nullopt) {
  // determine argument offset based on selection
  auto& value = args[arg_select];
  lang::memory::Ref ref = value.get().rvalue().ref();

  if (cast_to.has_value()) ref = ctx.reg_alloc_manager.guarantee_datatype(ref, cast_to.value());
  else ref = ctx.reg_alloc_manager.guarantee_register(ref);
  return ref.offset;
}

// fetch LHS and RHS argument pair, enforcing at least one is in a register
// return <register argument, other argument>
// argument: cast arguments to this type?
static std::pair<uint8_t, std::unique_ptr<lang::assembly::Arg>> fetch_argument_pair(lang::Context& ctx, Args args, optional_ref<const lang::ast::type::Node> cast_to = std::nullopt) {
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
    {"||", {2, false, false}},  // logical OR
    {"&&", {3, false, false}},  // logical AND

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
    {"[]", {19, false}},  // subscript
    {"()", {19, false}},  // function call
    {".", {20, false, false}},  // member access
};

std::unordered_map<std::string, const lang::ops::OperatorInfo> lang::ops::builtin_unary{
    {"-", {18, true}}, // negation
    {"!", {18, true}}, // logical NOT
    {"~", {18, true}}, // bitwise NOT
    {"&", {18, true, false}}, // address-of
    {"*", {18, true, false}}, // dereference
    {"sizeof", {18, true, false}}, // size of type
};

const lang::ops::OperatorInfo lang::ops::generic_binary{5, false};
const lang::ops::OperatorInfo lang::ops::generic_unary{18, true};

namespace generators {
  using namespace lang;

  // generate an addition instruction for the given asm datatype
  // assume values stored in LHS and RHS are compatible with the asm datatype
  // return which register the result is in
  static uint8_t generate_add(Context& ctx, Args args, const ast::type::Node& datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_add(datatype.get_asm_datatype(), reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for subtraction
  static uint8_t generate_sub(Context& ctx, Args args, const ast::type::Node& datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_sub(datatype.get_asm_datatype(), reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for multiplication
  static uint8_t generate_mul(Context& ctx, Args args, const ast::type::Node& datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_mul(datatype.get_asm_datatype(), reg_arg, reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // like generate_add, but for multiplication
  static uint8_t generate_div(Context& ctx, Args args, const ast::type::Node& datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_div(datatype.get_asm_datatype(), reg_arg, reg_arg, std::move(other_arg)));
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
    uint8_t reg = fetch_reg(ctx, args, 0);
    ctx.program.current().add(assembly::create_not(reg, reg));
    return reg;
  }

  // like generate_add, but for Boolean NOT
  static uint8_t generate_boolean_not(Context& ctx, Args args) {
    uint8_t reg = fetch_reg(ctx, args, 0);
    ctx.program.current().add(assembly::create_xor(reg, reg, assembly::Arg::imm(1)));
    return reg;
  }

  // like generate_add, but for a comparison
  static uint8_t generate_cmp(Context& ctx, Args args, const ast::type::Node& datatype) {
    auto [reg_arg, other_arg] = fetch_argument_pair(ctx, args, datatype);
    ctx.program.current().add(assembly::create_comparison(datatype.get_asm_datatype(), reg_arg, std::move(other_arg)));
    return reg_arg;
  }

  // generate a comparison, setting a Boolean result if equal to a test flag
  static uint8_t generate_cmp_bool(Context& ctx, Args args, std::reference_wrapper<const ast::type::Node> datatype, constants::cmp::flag cmp) {
    uint8_t reg = generate_cmp(ctx, args, datatype);
    // zero-out the register
    ctx.program.current().add(assembly::create_zero(reg));
    // set to true if flag matches
    ctx.program.current().add(set_conditional(assembly::create_load(reg, assembly::Arg::imm(1)), cmp));
    return reg;
  }

  // like generate_add, but for negation
  static uint8_t generate_neg(Context& ctx, Args args, const ast::type::Node& datatype) {
    uint8_t reg_arg = fetch_reg(ctx, args, 0, datatype);

    // load 0 into register
    // TODO make more efficient?
    // TODO how can we ensure both are in a register?
    const memory::Literal& zero = memory::Literal::zero(datatype);
    memory::Ref ref = ctx.reg_alloc_manager.guarantee_register(ctx.reg_alloc_manager.find_or_insert(zero));

    ctx.program.current().add(assembly::create_sub(datatype.get_asm_datatype(), reg_arg, ref.offset, assembly::Arg::reg(reg_arg)));
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
      // operator+(T, T)
      store_operator(std::make_unique<BuiltinOperator>(
          "+",
          FunctionNode::create({type, type}, type),
          [type](Context& ctx, Args args) { return generators::generate_add(ctx, args, type); }
      ));
    }
  }

  static void subtraction() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      // operator-(T, T)
      store_operator(std::make_unique<BuiltinOperator>(
          "-",
          FunctionNode::create({type, type}, type),
          [type](Context& ctx, Args args) { return generators::generate_sub(ctx, args, type); }
      ));
    }
  }

  static void multiplication() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      // operator*(T, T)
      store_operator(std::make_unique<BuiltinOperator>(
          "*",
          FunctionNode::create({type, type}, type),
          [type](Context& ctx, Args args) { return generators::generate_mul(ctx, args, type); }
      ));
    }
  }

  static void division() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      // operator/(T, T)
      store_operator(std::make_unique<BuiltinOperator>(
          "/",
          FunctionNode::create({type, type}, type),
          [type](Context& ctx, Args args) { return generators::generate_div(ctx, args, type); }
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
      for (const auto& type : numerical) {
        // operatorX(T, T)
        store_operator(std::make_unique<RelationalBuiltinOperator>(
            op,
            FunctionNode::create({type, type}, boolean),
            [cmp, type](Context& ctx, Args args) { return generators::generate_cmp_bool(ctx, args, type, cmp); },
            cmp,
            type
        ));
      }
    }

    // operator!=(bool, bool)
    store_operator(std::make_unique<RelationalBuiltinOperator>(
        "!=",
        FunctionNode::create({boolean, boolean}, boolean),
        [](Context& ctx, Args args) { return generators::generate_xor(ctx, args); },
        constants::cmp::ne,
        boolean
    ));

    // operator==(bool, bool)
    store_operator(std::make_unique<RelationalBuiltinOperator>(
        "==",
        FunctionNode::create({boolean, boolean}, boolean),
        [](Context& ctx, Args args) {
          uint8_t reg = generators::generate_xor(ctx, args);
          ctx.program.current().add(assembly::create_not(reg, reg));
          return reg;
        },
        constants::cmp::eq,
        boolean
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
    store_operator(std::make_unique<BooleanNotBuiltinOperator>(
        "!",
        FunctionNode::create({boolean}, boolean),
        [](Context& ctx, Args args) { return generators::generate_boolean_not(ctx, args); }
    ));
  }

  static void negation() {
    for (const auto& type : numerical) {
      if (type.get().size() < 4) continue; // skip smaller sizes
      // operator-(T)
      store_operator(std::make_unique<BuiltinOperator>(
          "-",
          FunctionNode::create({type}, type),
          [type](Context& ctx, Args args) { return generators::generate_neg(ctx, args, type); }
      ));
    }
  }

  // create logical && and ||
  static void logical_ops() {
    // operator&&(bool, bool)
    store_operator(std::make_unique<LazyLogicalOperator>(
        "&&",
        FunctionNode::create({boolean, boolean}, boolean),
        true
      ));

    // operator||(bool, bool)
    store_operator(std::make_unique<LazyLogicalOperator>(
        "||",
        FunctionNode::create({boolean, boolean}, boolean),
        false
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
  bitwise_not();
  boolean_not();
  relational();
  negation();
  logical_ops();
}

void lang::ops::boolean_cast(assembly::BasicBlock& block, uint8_t reg) {
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

void lang::ops::cast(lang::assembly::BasicBlock& block, uint8_t reg, constants::inst::datatype::dt original, constants::inst::datatype::dt target) {
  // exit if types are equal
  if (original == target) return;

  block.add(assembly::create_conversion(original, reg, target, reg));

  // add comment
  auto& comment = block.back().comment();
  ast::type::from_asm_type(original).print_code(comment);
  comment << " -> ";
  ast::type::from_asm_type(target).print_code(comment);
}

bool lang::ops::call_function(std::unique_ptr<assembly::BaseArg> function, const std::string& name,
                              const lang::ast::type::FunctionNode& signature,
                              const std::deque<std::unique_ptr<ast::Node>>& args,
                              const std::unordered_set<int>& args_to_ignore,
                              lang::value::Value& return_value,
                              optional_ref<const memory::StorageLocation> target, lang::Context& ctx) {
  // before entering the function: if reference to value, create buffer space to load value
  auto& return_type = signature.returns();
  bool has_return_buffer = false;
  if ((has_return_buffer = return_type.reference_as_ptr() && return_type.size() > 8 && !target)) {
    // reserve sufficient space
    ctx.stack_manager.push(return_type.size());
    auto& comment = ctx.program.current().back().comment();
    comment << "return buffer: ";
    return_type.print_code(comment);
  }

  // record offset at start of the whole process
  const uint64_t start_offset = ctx.stack_manager.offset();

  // save registers
  ctx.reg_alloc_manager.save_store(true);

  // push $rpc
  ctx.stack_manager.push(8);
  ctx.program.current().back().comment() << "save $rpc";
  ctx.program.current().add(assembly::create_store(
      constants::registers::rpc,
      assembly::Arg::reg_indirect(constants::registers::sp)
  ));

  // push $fp
  ctx.stack_manager.push(8);
  ctx.program.current().back().comment() << "save $fp";
  ctx.program.current().add(assembly::create_store(
      constants::registers::fp,
      assembly::Arg::reg_indirect(constants::registers::sp)
  ));

  // save $fp
  ctx.stack_manager.push_frame(true);

  // push arguments
  int i = 0;
  for (const auto& arg : args) {
    // evaluate rvalue
    auto& value = arg->value();
    arg->type_hint(signature.arg(i)); // given arg a type hint
    if (!arg->generate_code(ctx)) return false;
    value.materialise(ctx, arg->token_start().loc);
    if (!value.is_rvalue()) {
      ctx.messages.add(util::error_expected_lrvalue(*arg, value.type(), false));
      return false;
    }

    // if size==0, or ignore, skip (as nothing actually there)
    if (value.type().size() == 0 || args_to_ignore.contains(i)) continue;

    // get location of the argument
    const memory::Ref location = value.rvalue().ref();

    // create necessary space on the stack
    size_t bytes = value.type().size();
    ctx.stack_manager.push(bytes);
    auto& comment =  ctx.program.current().back().comment();
    comment << "arg #" << (i + 1) << ": ";
    value.type().print_code(comment);

    // if reference-like, we must copy
    if (value.type().reference_as_ptr()) {
      mem_copy(ctx, location, value, assembly::Arg::reg(constants::registers::sp));
    } else {
      // store data in register here
      assembly::create_store(
          location.offset,
          assembly::Arg::reg_indirect(constants::registers::sp),
          bytes,
          ctx.program.current()
      );
    }

    // we are done with this register now
    ctx.reg_alloc_manager.mark_free(location);
    i++;
  }

  // call the function (via jal) and add comment
  ctx.program.current().add(assembly::create_jump_and_link(std::move(function)));
  auto& comment = ctx.program.current().back().comment();
  comment << "call " << name;
  signature.print_code(comment);

  // update value_'s location
  return_value.rvalue(memory::Ref::reg(constants::registers::ret));

  // restore $sp
  ctx.stack_manager.pop_frame(true);

  // restore $fp
  ctx.program.current().add(assembly::create_load(
      constants::registers::fp,
      assembly::Arg::reg_indirect(constants::registers::fp)
  ));
  ctx.program.current().back().comment() << "restore $fp";

  // restore $rpc
  ctx.program.current().add(assembly::create_load(
      constants::registers::rpc,
      assembly::Arg::reg_indirect(constants::registers::fp, 8)
  ));
  ctx.program.current().back().comment() << "restore $rpc";

  // restore registers
  ctx.reg_alloc_manager.destroy_store(true);

  // populate $ret in alloc manager
  ctx.reg_alloc_manager.update_ret(memory::Object(value::rvalue(
      signature.returns(),
      memory::Ref::reg(constants::registers::ret)
    )));

  // restore $sp to starting position
  if (auto current_offset = ctx.stack_manager.offset(); current_offset != start_offset) {
    ctx.program.current().add(assembly::create_add(
        constants::inst::datatype::u64,
        constants::registers::sp,
        constants::registers::sp,
        assembly::Arg::imm(current_offset - start_offset)
    ));
    ctx.program.current().back().comment() << "stack clean-up ";
  }

  // copy return value into the return buffer?
  if (has_return_buffer) {
    // mem copy
    const int index = ctx.program.current().size();
    ops::mem_copy(
        ctx,
        memory::Ref::reg(constants::registers::ret),
        return_value,
        assembly::Arg::reg(constants::registers::sp)
      );

    auto& comment = ctx.program.current()[index].comment();
    comment << "copy return value into buffer";
  }

  return true;
}

int lang::ops::pointer_arithmetic(assembly::BasicBlock& block, uint8_t reg_a, uint8_t reg_b, uint32_t imm_c, bool is_subtraction) {
  const auto& create_arith = is_subtraction
      ? assembly::create_sub
      : assembly::create_add;

  if (imm_c > 2) { // if this big, we need to scale first
    // b := b * c
    block.add(assembly::create_mul(
        constants::inst::datatype::u64,
        reg_b,
        reg_b,
        assembly::Arg::imm(imm_c)
    ));
  } else if (imm_c == 2) { // just hardcode a double addition
    // a := a + b
    block.add(create_arith(
        constants::inst::datatype::u64,
        reg_a,
        reg_a,
        assembly::Arg::reg(reg_b)
    ));
  }

  // a := a + b
  block.add(create_arith(
      constants::inst::datatype::u64,
      reg_a,
      reg_a,
      assembly::Arg::reg(reg_b)
  ));

  return imm_c > 2 ? 2 : 1;
}

lang::memory::Ref lang::ops::pointer_arithmetic(Context& ctx, const value::Value& pointer, const value::Value& offset, bool is_subtraction, bool add_comment) {
  assert(pointer.type().get_pointer() || pointer.type().get_array());
  assert(pointer.is_rvalue() && offset.is_rvalue());

  const auto ref_ptr = ctx.reg_alloc_manager.guarantee_register(pointer.rvalue().ref());
  const auto ref_offset = ctx.reg_alloc_manager.guarantee_register(offset.rvalue().ref());
  int idx = ctx.program.current().size();

  // generate instructions, return number of registers to 'spoil'
  int to_spoil = ops::pointer_arithmetic(
      ctx.program.current(),
      ref_ptr.offset,
      ref_offset.offset,
      pointer.type().get_wrapper()->unwrap().size(),
      is_subtraction
  );

  // add comment
  if (add_comment) {
    auto& comment = ctx.program.current()[idx].comment();
    comment << "operator" << (is_subtraction ? "-" : "+") << "(";
    pointer.type().print_code(comment) << ", ";
    offset.type().print_code(comment) << ")";
  }

  // spoil `ptr` register
  auto& object_ptr = ctx.reg_alloc_manager.find(ref_ptr);
  object_ptr.value->rvalue(std::make_unique<value::RValue>(pointer.type(), ref_ptr));

  if (to_spoil == 2) {
    // spoil `offset` register
    auto& object_offset = ctx.reg_alloc_manager.find(ref_offset);
    object_offset.value->rvalue(std::make_unique<value::RValue>(ast::type::uint64, ref_offset));
  }

  return ref_ptr;
}

void lang::ops::dereference(Context& ctx, const value::Value& pointer, value::Value& result, bool add_comment) {
  assert(pointer.is_rvalue());
  assert(pointer.type().get_pointer() || pointer.type().get_array());

  // get the source (pointer location)
  memory::Ref src = pointer.rvalue().ref();
  src = ctx.reg_alloc_manager.guarantee_register(src);

  // update result to point to the original
  result.lvalue(src);

  // special case: if pointer to pointer-like, do not dereference
  memory::Ref dest;
  if (auto pointer_type = pointer.type().get_pointer(); pointer_type && pointer_type->unwrap().reference_as_ptr()) {
    dest = src;
  } else {
    // reserve a register to place the dereferenced result
    dest = ctx.reg_alloc_manager.insert({nullptr});
    dest = ctx.reg_alloc_manager.guarantee_register(dest);

    // load into register
    ctx.program.current().add(assembly::create_load(
        dest.offset,
        assembly::Arg::reg_indirect(src.offset)
    ));

    // add comment
    if (add_comment) {
      auto& comment = ctx.program.current().back().comment();
      comment << "deref ";
      pointer.type().print_code(comment);
    }
  }

  // update register allocation
  const auto& deref_type = pointer.type().get_wrapper()->unwrap();
  memory::Object& object = ctx.reg_alloc_manager.find(dest);
  object.value = value::rvalue(deref_type, dest);

  // update resulting value's rvalue
  result.rvalue(std::make_unique<value::RValue>(deref_type, dest));
}

bool lang::ops::address_of(lang::Context& ctx, const lang::symbol::Symbol& symbol, lang::value::Value& result) {
  // get location of the symbol (must have size then)
  auto maybe_location = ctx.symbols.locate(symbol.id());
  if (!maybe_location) return false;

  // find destination register
  memory::Ref ref = ctx.reg_alloc_manager.insert({nullptr});
  ref = ctx.reg_alloc_manager.guarantee_register(ref);

  // get address and store in register
  const memory::StorageLocation& location = maybe_location->get();

  if (location.type == memory::StorageLocation::Stack) { // calculate offset from $sp
    const uint64_t stack_offset = ctx.stack_manager.offset();

    if (location.base_offset == stack_offset) {
      ctx.program.current().add(assembly::create_load(
          ref.offset,
          assembly::Arg::reg(constants::registers::sp)
      ));
    } else if (location.base_offset < stack_offset) {
      ctx.program.current().add(assembly::create_sub(
          constants::inst::datatype::u64,
          ref.offset,
          constants::registers::sp,
          assembly::Arg::imm(stack_offset - location.base_offset)
      ));
    } else {
      ctx.program.current().add(assembly::create_add(
          constants::inst::datatype::u64,
          ref.offset,
          constants::registers::sp,
          assembly::Arg::imm(location.base_offset - location.base_offset)
      ));
    }
  } else {
    ctx.program.current().add(assembly::create_load(
        ref.offset,
        assembly::Arg::label(location.block)
    ));
  }

  // update value_ & object
  memory::Object& object = ctx.reg_alloc_manager.find(ref);
  object.value = value::rvalue(result.type(), ref);
  result.rvalue(ref);
  return true;
}

void lang::ops::mem_copy(Context& ctx, const memory::Ref& src, const value::Value& dest, std::unique_ptr<assembly::BaseArg> dest_arg) {
  assert(src.type == memory::Ref::Register);
  assert(dest.is_lvalue() || dest_arg != nullptr);

  auto& block = ctx.program.current();
  std::string into_comment;

  // source address (RHS)
  uint8_t reg = constants::registers::syscall_start;
  std::optional<memory::Object> old_r1;
  if (src.offset != reg) {
    old_r1 = ctx.reg_alloc_manager.save_register(reg);
    block.add(assembly::create_load(
        reg,
        assembly::Arg::reg(src.offset)
    ));
  }

  // next, load the designation address (LHS)
  // if LHS a symbol?
  reg++;
  std::optional<memory::Object> old_r2;
  if (dest_arg) {
    // use provided argument in `load`
    old_r2 = ctx.reg_alloc_manager.save_register(reg);
    block.add(assembly::create_load(
        reg,
        std::move(dest_arg)
    ));
  } else if (auto symbol = dest.lvalue().get_symbol()) {
    // get symbol and its location
    auto location = ctx.symbols.locate(symbol->get().id());
    assert(location.has_value());

    // comment symbol's name
    into_comment = symbol->get().full_name();

    // source address
    if (auto maybe = ctx.reg_alloc_manager.find(symbol->get()); !maybe.has_value() ||
                                                                *maybe != memory::Ref::reg(reg)) {
      old_r2 = ctx.reg_alloc_manager.save_register(reg);
      block.add(assembly::create_load(
          reg,
          location->get().resolve()
      ));
    }
  } else {
    // get LHS' reference
    auto lvalue_ref = dest.lvalue().get_ref();
    assert(lvalue_ref);
    uint64_t offset = lvalue_ref->get().offset;

    // comment an intermediate
    into_comment = "$" + constants::registers::to_string($reg(offset));

    if (offset != reg) {
      old_r2 = ctx.reg_alloc_manager.save_register(reg);
      block.add(assembly::create_load(
          reg,
          assembly::Arg::reg(offset)
      ));
    }
  }

  // length
  reg++;
  auto old_r3 = ctx.reg_alloc_manager.save_register(reg);
  block.add(assembly::create_load(
      reg,
      assembly::Arg::imm(dest.type().size())
  ));

  // invoke syscall
  block.add(assembly::create_system_call(
      assembly::Arg::imm(static_cast<uint32_t>(constants::syscall::copy_mem))
  ));

  // add comment
  auto& comment = block.back().comment();
  comment << "mem_copy";
  if (!into_comment.empty()) comment << " into " << into_comment;
  comment << ": ";
  dest.type().print_code(comment);

  // restore registers?
  reg = constants::registers::syscall_start;
  if (old_r3.has_value()) ctx.reg_alloc_manager.restore_register(reg + 2, old_r3.value());
  if (old_r2.has_value()) ctx.reg_alloc_manager.restore_register(reg + 1, old_r2.value());
  if (old_r1.has_value()) ctx.reg_alloc_manager.restore_register(reg, old_r1.value());
}
