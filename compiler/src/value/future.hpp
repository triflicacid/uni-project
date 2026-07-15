#pragma once

#include "value.hpp"
#include "symbol/table.hpp"
#include "types/unit.hpp"

namespace lang::value {
  /**
   * @brief An unresolved symbol-name reference: a name plus a set of candidate symbols it might refer to.
   *
   * Not itself computable; used when a name may still match multiple overloads
   * or shadowed declarations. resolve() narrows the candidate set down to one,
   * at which point it gains a value::Symbol lvalue.
   */
  // represents a symbol name (reference)
  // we are not computable, either - if we were, return a Symbol
  // therefore, this is generally used when we have multiple options to pick from
  class SymbolRef : public Value {
    std::string name_;
    std::deque<std::reference_wrapper<symbol::Symbol>> overload_set_;

  public:
    /**
     * @brief Constructs an unresolved reference from a name and its candidate symbol set.
     * @param name Name this reference was built from.
     * @param overload_set Candidate symbols the name may refer to.
     */
    SymbolRef(std::string name, std::deque<std::reference_wrapper<symbol::Symbol>> overload_set) : name_(std::move(name)), overload_set_(std::move(overload_set)) {}

    /**
     * @brief Returns the referenced name.
     * @return The name.
     */
    const std::string& get() const { return name_; }

    /**
     * @brief Confirms this value is a SymbolRef.
     * @return This.
     */
    const SymbolRef* get_symbol_ref() const override { return this; }

    /**
     * @brief Produces an independent duplicate, including its resolved lvalue/rvalue if present.
     * @return The copied value.
     */
    std::unique_ptr<Value> copy() const override;

    /**
     * @brief Returns the current candidate symbol set.
     * @return The candidates.
     */
    // return our overload set (possible symbols we may adopt)
    const std::deque<std::reference_wrapper<symbol::Symbol>>& candidates() const { return overload_set_; }

    /**
     * @brief Narrows the candidate set down to exactly one symbol, optionally using a type hint, and installs it as this value's lvalue on success.
     * @param source Location to attribute any diagnostics to.
     * @param messages Optional message list to append diagnostics to; if omitted, resolution fails silently.
     * @param type_hint Optional type used to disambiguate between multiple candidates of the same name.
     * @return True if resolution narrowed to exactly one symbol.
     */
    // attempt to resolve this symbol, populates lvalue with ::Symbol
    bool resolve(const message::MessageGenerator &source, optional_ref<message::List> messages, optional_ref<const type::Node> type_hint = {});

    /**
     * @brief Loads the resolved symbol's value into a register and optionally copies it into a target location.
     * @param ctx Compiler context.
     * @param options Materialisation parameters.
     * @return True if a copy into options.target was performed, or if the symbol has no recorded storage location; false otherwise. Requires resolve() to have already succeeded.
     */
    bool materialise(Context &ctx, const MaterialisationOptions &options) override;
  };

  /**
   * @brief Creates a SymbolRef value, populating its candidate set from a symbol table lookup by name.
   * @param name Name to look up.
   * @param symbols Symbol table to search.
   * @return The newly created symbol reference.
   */
  // create a Value which is a symbol reference, populate overload set from ctx
  std::unique_ptr<SymbolRef> symbol_ref(const std::string name, const symbol::SymbolTable& symbols);

  /**
   * @brief A compile-time scalar constant that resolves into a literal of its type, restricted to word-sized (non-reference) types.
   */
  // we will resolve into a literal of the given type
  // this is for word-sized literals only (non-reference types)
  class Literal : public Value {
    const memory::Literal& lit_;

  public:
    /**
     * @brief Constructs a value-layer wrapper around a raw interned literal.
     * @param lit Literal to wrap.
     */
    explicit Literal(const memory::Literal& lit) : Value(lit.type()), lit_(lit) {}

    /**
     * @brief Produces an independent duplicate wrapping the same interned literal, including its rvalue/lvalue if present.
     * @return The copied value.
     */
    std::unique_ptr<Value> copy() const override;

    /**
     * @brief Returns the wrapped raw literal.
     * @return The literal.
     */
    const memory::Literal& get() const { return lit_; }

    /**
     * @brief Confirms this value is a Literal.
     * @return This.
     */
    const Literal* get_literal() const override { return this; }

    /**
     * @brief Loads the literal's value into a register and, if requested, stores or copies it into a target location.
     * @param ctx Compiler context.
     * @param options Materialisation parameters.
     * @return Always true once materialisation completes.
     */
    bool materialise(Context &ctx, const MaterialisationOptions &options = {}) override;
  };

  /**
   * @brief Creates a value::Literal wrapper around a raw literal.
   * @param lit Literal to wrap.
   * @return The newly created value.
   */
  // create a literal
  std::unique_ptr<Value> literal(const memory::Literal& lit);

  /**
   * @brief Creates a value::Literal that is already materialised at a known location.
   * @param lit Literal to wrap.
   * @param ref Reference identifying the literal's location.
   * @return The newly created value.
   */
  // create a literal rvalue
  std::unique_ptr<Value> literal(const memory::Literal& lit, const memory::Ref& ref);

  /**
   * @brief A sequence of word-sized literal elements laid out contiguously in memory, e.g. the backing data of an array literal.
   */
  // we represent a sequence of literals which will be placed in contiguous memory
  // this is for word-sized literals only (non-reference types)
  class ContiguousLiteral : public Value {
  public:
    using Elements = std::deque<std::reference_wrapper<value::Value>>;

  private:
    Elements elements_;
    bool global_; // define globally or locally?

  public:
    /**
     * @brief Constructs a contiguous-literal aggregate value.
     * @param type Static type of the aggregate.
     * @param elements Element values to lay out contiguously, in order.
     * @param is_global Whether the aggregate's backing storage should be a global data block rather than stack-allocated.
     */
    ContiguousLiteral(const type::Node& type, Elements elements, bool is_global);

    /**
     * @brief Produces an independent duplicate sharing the same underlying element values, including its rvalue/lvalue if present.
     * @return The copied value.
     */
    std::unique_ptr<Value> copy() const override;

    /**
     * @brief Lays out every element contiguously at a (possibly newly-allocated) base location, then, if outermost, loads a reference to the whole block.
     * @param ctx Compiler context.
     * @param options Materialisation parameters.
     * @return True if every element materialised successfully.
     */
    bool materialise(Context &ctx, const MaterialisationOptions &options = {}) override;
  };

  /**
   * @brief Creates a ContiguousLiteral value.
   * @param type Static type of the aggregate.
   * @param elements Element values to lay out contiguously.
   * @param is_global Whether the aggregate's backing storage should be a global data block rather than stack-allocated.
   * @return The newly created value.
   */
  // create a literal
  std::unique_ptr<Value> contiguous_literal(const type::Node& type, ContiguousLiteral::Elements elements, bool is_global);
}
