#pragma once

#include <functional>
#include "operator.hpp"
#include "constants.hpp"
#include "value/value.hpp"
#include "memory/storage_location.hpp"
#include "assembly/arg.hpp"
#include <unordered_set>

namespace lang {
  struct Context;
}

namespace lang::ast {
  class Node;
}

namespace lang::assembly {
  class BasicBlock;
}

namespace lang::ops {
  class BuiltinOperator : public Operator {
    using GeneratorFn = std::function<uint8_t(Context&, const std::deque<std::reference_wrapper<const value::Value>>&)>;

    // function to generate appropriate code
    // provided array of arguments -- left to right
    // returns result
    GeneratorFn generator_;

  public:
    BuiltinOperator(std::string symbol, const ast::type::FunctionNode& type, const GeneratorFn& generator)
      : Operator(std::move(symbol), type), generator_(generator) {}

    bool builtin() const override { return true; }

    // get the attached function
    const GeneratorFn& generator() const { return generator_; }
  };

  // initialise builtins
  void init_builtins();

  // generate a conversion for `reg` to the target type
  // (if types are equal, do nothing)
  void cast(assembly::BasicBlock& block, uint8_t reg, constants::inst::datatype::dt original, constants::inst::datatype::dt target);

  // generate code for a Boolean cast, but does not update the register
  void boolean_cast(assembly::BasicBlock& block, uint8_t reg);

  // generate a call to function pointed to by argument
  // save stack frame, registers, prepare args, etc.
  // return if success
  bool call_function(std::unique_ptr<assembly::BaseArg> function, const std::string& name, const ast::type::FunctionNode& signature, const std::deque<std::unique_ptr<ast::Node>>& args, const std::unordered_set<int>& args_to_ignore, value::Value& return_value, Context& ctx);

  // generate a call to an operator, return if success
  // note, does not assign type to return_value
  bool call_operator(Context& ctx, const std::string& name, const ops::Operator& op, const std::deque<std::unique_ptr<ast::Node>>& args, value::Value& return_value);

  // pointer addition: r = a <op> b * c where `a` is ptr, `b` is offset, `c` is size
  // <op> is either `add` or `sub` depending on flag
  // registers are u64, result is placed in reg_a
  // return registers to soil: 1 if just reg_a, 2 is both registers
  int pointer_arithmetic(assembly::BasicBlock& block, uint8_t reg_a, uint8_t reg_b, uint32_t imm_c, bool is_subtraction);

  // higher-level ops::pointer_arithmetic using Values
  // assume first value is a pointer, second is a u64 subtype
  // return a reference to the result
  memory::Ref pointer_arithmetic(Context& ctx, const value::Value& pointer, const value::Value& offset, bool is_subtraction);

  // de-reference a pointer value into the second
  // should we add a 'deref ...' comment?
  void dereference(Context& ctx, const value::Value& pointer, value::Value& result, bool add_comment);
}
