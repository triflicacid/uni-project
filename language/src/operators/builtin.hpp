#pragma once

#include <functional>
#include "operator.hpp"
#include "constants.hpp"
#include "value/value.hpp"

namespace lang {
  struct Context;
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
}
