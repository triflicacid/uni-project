#pragma once

#include <deque>
#include <unordered_set>
#include "constants.hpp"
#include "assembly/arg.hpp"
#include "memory/ref.hpp"
#include "memory/storage_location.hpp"
#include "optional_ref.hpp"

namespace lang {
  struct Context;

  namespace assembly {
    class BasicBlock;
  }

  namespace ast {
    class Node;

    namespace type {
      class FunctionNode;
    }
  }

  namespace ops {
    class Operator;
  }

  namespace symbol {
    class Symbol;
  }

  namespace value {
    class Value;
  }
}

namespace lang::ops {
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
  bool call_function(std::unique_ptr<assembly::BaseArg> function, const std::string& name, const ast::type::FunctionNode& signature, const std::deque<std::unique_ptr<ast::Node>>& args, const std::unordered_set<int>& args_to_ignore, value::Value& return_value, optional_ref<const memory::StorageLocation> target, Context& ctx);

  // pointer addition: r = a <op> b * c where `a` is ptr, `b` is offset, `c` is size
  // <op> is either `add` or `sub` depending on flag
  // registers are u64, result is placed in reg_a
  // return registers to soil: 1 if just reg_a, 2 is both registers
  int pointer_arithmetic(assembly::BasicBlock& block, uint8_t reg_a, uint8_t reg_b, uint32_t imm_c, bool is_subtraction);

  // higher-level ops::pointer_arithmetic using Values
  // assume first value is a pointer or array, second is a u64 subtype
  // return a reference to the result
  memory::Ref pointer_arithmetic(Context& ctx, const value::Value& pointer, const value::Value& offset, bool is_subtraction, bool add_comment);

  // de-reference a pointer value into the second
  // should we add a 'deref ...' comment?
  void dereference(Context& ctx, const value::Value& pointer, value::Value& result, bool add_comment);

  // get the address of the given symbol and place it in the value
  // returns if success, as this can error (e.g., if the symbol cannot be found)
  // note that no message is generated on error
  bool address_of(Context& ctx, const symbol::Symbol& symbol, value::Value& result);

  // mem copy a reference into a value
  // assume `src` is a register
  // if provided, use `dest_arg`, otherwise expect `dest` to be an lvalue and copy into there
  void mem_copy(Context& ctx, const memory::Ref& src, const value::Value& dest, std::unique_ptr<assembly::BaseArg> dest_arg);
}
