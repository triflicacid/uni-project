#pragma once

#include <functional>
#include "operator.hpp"
#include "constants.hpp"

namespace lang {
  struct Context;
}

namespace lang::ops {
  class BuiltinOperator : public Operator {
    // function to generate appropriate code
    // we use RegisterAllocationManager::get_recent to fetch arguments - RHS is most recent (n=0), LHS is next (n=1)
    std::function<void(Context&)> generator_;

  public:
    BuiltinOperator(std::string symbol, const ast::type::FunctionNode& type, const std::function<void(Context&)>& generator)
      : Operator(std::move(symbol), type), generator_(generator) {}

    bool builtin() const override { return true; }

    // call our underlying generator to create assembly code
    void process(Context& ctx) const;
  };

  // initialise builtins
  void init_builtins();

  // generate a conversion for the current register to `target`
  // return if instruction was emitted
  bool implicit_cast(Context& ctx, constants::inst::datatype::dt target);
}
