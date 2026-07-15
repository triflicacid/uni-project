#pragma once

#include <memory>
#include <cassert>
#include "lvalue.hpp"
#include "rvalue.hpp"
#include "optional_ref.hpp"
#include "messages/list.hpp"
#include "materialisation_options.hpp"

namespace lang {
  struct Context;
}

namespace lang::type {
  class Node;
}

/** @brief Compile-time representation of values produced by evaluating an expression: lvalues, rvalues, and their eventual materialisation into a concrete storage location. */
namespace lang::value {
  class SymbolRef;
  class Literal;

  /**
   * @brief The result of evaluating an AST node: an optional addressable-storage (lvalue) component and/or an optional concrete-location (rvalue) component, plus a static type.
   *
   * A value may be an lvalue only, an rvalue only, both, or neither (not yet
   * resolved/materialised). This lets the same type flow through every compiler
   * phase without committing early to whether a node's result is a variable,
   * literal, or computed temporary.
   */
  class Value {
    std::unique_ptr<LValue> lvalue_; ///< Addressable-storage component, if applicable.
    std::unique_ptr<RValue> rvalue_; ///< Concrete-location component, if applicable.
    std::reference_wrapper<const type::Node> type_; ///< Static type of the value.

  public:
    /**
     * @brief Constructs an empty, unit-typed value.
     */
    Value();

    /**
     * @brief Constructs an empty value of a given type.
     * @param type Static type of the value.
     */
    Value(const type::Node& type);

    /**
     * @brief Produces a deep, independent duplicate of this value, including its lvalue and rvalue components if present.
     * @return The copied value.
     */
    // copy ourself deeply (copies l/rvalue components)
    virtual std::unique_ptr<Value> copy() const;

    /**
     * @brief Returns the value's static type.
     * @return The type.
     */
    const type::Node& type() const { return type_; }

    /**
     * @brief Reports whether this value has an addressable-storage component.
     * @return True if an lvalue is attached.
     */
    bool is_lvalue() const { return lvalue_ != nullptr; }

    /**
     * @brief Accesses the lvalue component.
     * @return The lvalue. Behaviour is undefined (asserted in debug builds) if none is attached.
     */
    LValue& lvalue() const;

    /**
     * @brief Installs a new lvalue component, taking ownership, and syncs the value's type to it.
     * @param v Lvalue to attach.
     */
    void lvalue(std::unique_ptr<LValue> v);

    /**
     * @brief Binds this value to a symbol as its lvalue.
     * @param symbol Symbol to bind to.
     */
    void lvalue(const symbol::Symbol& symbol);

    /**
     * @brief Binds this value to a raw memory/register reference as its lvalue.
     * @param ref Reference to bind to.
     */
    void lvalue(const memory::Ref& ref);

    /**
     * @brief Reports whether this value currently has a concrete location holding its computed value.
     * @return True if an rvalue is attached.
     */
    bool is_rvalue() const { return rvalue_ != nullptr; }

    /**
     * @brief Accesses the rvalue component.
     * @return The rvalue. Behaviour is undefined (asserted in debug builds) if none is attached.
     */
    RValue& rvalue() const;

    /**
     * @brief Installs a new rvalue component, taking ownership, and syncs the value's type to it.
     * @param v Rvalue to attach.
     */
    void rvalue(std::unique_ptr<RValue> v);

    /**
     * @brief Attaches an rvalue at a given reference, reusing the value's current type.
     * @param ref Reference identifying the value's current location.
     */
    void rvalue(const memory::Ref& ref);

    /**
     * @brief Forces this value into a concrete register/location, optionally copying/moving it into a target storage location.
     * @param ctx Compiler context.
     * @param options Materialisation parameters: optional target location, copy-vs-move flag, and source origin.
     * @return Whether data was written into options.target specifically; the base implementation always returns false.
     */
    // attempt to materialise into an rvalue, return if any value was actually stored
    virtual bool materialise(Context& ctx, const MaterialisationOptions& options = {})
    { return false; }

    /**
     * @brief Convenience overload of materialise() that only supplies a source location for line-origin tracking.
     * @param ctx Compiler context.
     * @param origin Source location to attribute generated instructions to.
     * @return Same contract as the full materialise() overload.
     */
    // materialise with no options, but providing a location
    bool materialise(Context& ctx, const Location& origin);

    /**
     * @brief Down-casts this value to a SymbolRef if it is one.
     * @return This value as a SymbolRef, or nullptr.
     */
    virtual const SymbolRef* get_symbol_ref() const { return nullptr; }

    /**
     * @brief Down-casts this value to a Literal if it is one.
     * @return This value as a Literal, or nullptr.
     */
    virtual const Literal* get_literal() const { return nullptr; }
  };

  /**
   * @brief Creates a bare, empty value, optionally typed.
   * @param type Optional static type; defaults to unit if omitted.
   * @return The newly created value.
   */
  // create a generic value
  std::unique_ptr<Value> value(optional_ref<const type::Node> type = std::nullopt);

  /**
   * @brief Creates a value that is immediately an rvalue at a known location.
   * @param type Static type of the value.
   * @param ref Reference identifying the value's location.
   * @return The newly created value.
   */
  // create an rvalue
  std::unique_ptr<Value> rvalue(const type::Node& type, const memory::Ref& ref);

  /**
   * @brief Builds the canonical "no value" placeholder, a unit-typed rvalue with a sentinel register.
   * @return The newly created unit value.
   */
  // create a unit value
  std::unique_ptr<Value> unit_value();

  extern const std::unique_ptr<Value> unit_value_instance; ///< Shared value with unit type, used to signify "empty" or "none".
}
