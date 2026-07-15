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
  }

  namespace type {
    class FunctionNode;
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
  /**
   * @brief Registers every concrete builtin Operator instance into the global operator registry.
   */
  // initialise builtins
  void init_builtins();

  /**
   * @brief Emits a datatype conversion for a register's value, if its current and target datatypes differ.
   * @param block Basic block to append the conversion instruction to.
   * @param reg Register holding the value to convert.
   * @param original Current datatype of the value.
   * @param target Datatype to convert to.
   */
  // generate a conversion for `reg` to the target type
  // (if types are equal, do nothing)
  void cast(assembly::BasicBlock& block, uint8_t reg, constants::inst::datatype::dt original, constants::inst::datatype::dt target);

  /**
   * @brief Emits code coercing a register's value into a boolean, without updating any tracked register bookkeeping.
   * @param block Basic block to append instructions to.
   * @param reg Register holding the value to coerce.
   */
  // generate code for a Boolean cast, but does not update the register
  void boolean_cast(assembly::BasicBlock& block, uint8_t reg);

  /**
   * @brief Generates a full call to a function, saving the stack frame and registers, marshalling arguments, and retrieving the result.
   * @param function Callable assembly operand identifying the function's address.
   * @param name Qualified name of the function, used in diagnostics/comments.
   * @param signature Function's declared signature.
   * @param args Argument AST nodes, not yet code-generated.
   * @param args_to_ignore Indices of arguments to skip generating (already handled by the caller).
   * @param return_value Output value populated with the call's result.
   * @param target Optional storage location hint for where the result should end up.
   * @param ctx Compiler context.
   * @return True on success.
   */
  // generate a call to function pointed to by argument
  // save stack frame, registers, prepare args, etc.
  // return if success
  bool call_function(std::unique_ptr<assembly::BaseArg> function, const std::string& name, const type::FunctionNode& signature, const std::deque<std::unique_ptr<ast::Node>>& args, const std::unordered_set<int>& args_to_ignore, value::Value& return_value, optional_ref<const memory::StorageLocation> target, Context& ctx);

  /**
   * @brief Emits pointer arithmetic: computes reg_a = reg_a (+/-) reg_b * imm_c.
   * @param block Basic block to append instructions to.
   * @param reg_a Register holding the pointer; also receives the result.
   * @param reg_b Register holding the element offset.
   * @param imm_c Element size to scale the offset by.
   * @param is_subtraction True to subtract the scaled offset, false to add it.
   * @return Which registers were soiled: 1 if only reg_a, 2 if both reg_a and reg_b.
   */
  // pointer addition: r = a <op> b * c where `a` is ptr, `b` is offset, `c` is size
  // <op> is either `add` or `sub` depending on flag
  // registers are u64, result is placed in reg_a
  // return registers to soil: 1 if just reg_a, 2 is both registers
  int pointer_arithmetic(assembly::BasicBlock& block, uint8_t reg_a, uint8_t reg_b, uint32_t imm_c, bool is_subtraction);

  /**
   * @brief Higher-level pointer arithmetic operating on resolved values rather than raw registers.
   * @param ctx Compiler context.
   * @param pointer Pointer or array-typed value being offset.
   * @param offset Offset value, of a u64 subtype.
   * @param is_subtraction True to subtract the offset, false to add it.
   * @param add_comment Whether to annotate the emitted instruction with a descriptive comment.
   * @return Reference to the resulting pointer value.
   */
  // higher-level ops::pointer_arithmetic using Values
  // assume first value is a pointer or array, second is a u64 subtype
  // return a reference to the result
  memory::Ref pointer_arithmetic(Context& ctx, const value::Value& pointer, const value::Value& offset, bool is_subtraction, bool add_comment);

  /**
   * @brief Dereferences a pointer value into a result value.
   * @param ctx Compiler context.
   * @param pointer Pointer value to dereference.
   * @param result Output value populated with the dereferenced result.
   * @param add_comment Whether to annotate the emitted instruction with a "deref ..." comment.
   */
  // de-reference a pointer value into the second
  // should we add a 'deref ...' comment?
  void dereference(Context& ctx, const value::Value& pointer, value::Value& result, bool add_comment);

  /**
   * @brief Computes the address of a symbol and places it in a result value.
   * @param ctx Compiler context.
   * @param symbol Symbol to take the address of.
   * @param result Output value populated with the address.
   * @return True on success; false if the symbol's storage cannot be found (no diagnostic is generated on failure).
   */
  // get the address of the given symbol and place it in the value
  // returns if success, as this can error (e.g., if the symbol cannot be found)
  // note that no message is generated on error
  bool address_of(Context& ctx, const symbol::Symbol& symbol, value::Value& result);

  /**
   * @brief Copies a register-resident value into a destination, either an explicit operand or the destination value's lvalue location.
   * @param ctx Compiler context.
   * @param src Reference to the source register.
   * @param dest Destination value; used as the copy target's lvalue location if dest_arg is null.
   * @param dest_arg Optional explicit destination operand, overriding dest's own location.
   */
  // mem copy a reference into a value
  // assume `src` is a register
  // if provided, use `dest_arg`, otherwise expect `dest` to be an lvalue and copy into there
  void mem_copy(Context& ctx, const memory::Ref& src, const value::Value& dest, std::unique_ptr<assembly::BaseArg> dest_arg);
}
