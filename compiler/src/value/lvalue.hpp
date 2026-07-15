#pragma once

#include "memory/ref.hpp"

namespace lang::type {
  class Node;
}

namespace lang::symbol {
  class Symbol;
}

namespace lang::value {
  class Symbol;
  class Reference;

  /**
   * @brief Base class for the lvalue half of a Value: something with addressable storage.
   */
  // an lvalue is something with storage
  class LValue {
    const type::Node& type_;

  public:
    /**
     * @brief Constructs an lvalue bound to a static type.
     * @param type Static type of the lvalue.
     */
    explicit LValue(const type::Node& type) : type_(type) {}

    /**
     * @brief Returns the lvalue's static type.
     * @return The type.
     */
    const type::Node& type() const { return type_; }

    /**
     * @brief Produces an independent duplicate of this lvalue.
     * @return The copied lvalue.
     */
    virtual std::unique_ptr<LValue> copy() const;

    /**
     * @brief Down-casts this lvalue to a Symbol if it is one.
     * @return This lvalue as a Symbol, or nullptr.
     */
    virtual const Symbol* get_symbol() const { return nullptr; }

    /**
     * @brief Down-casts this lvalue to a Reference if it is one.
     * @return This lvalue as a Reference, or nullptr.
     */
    virtual const Reference* get_ref() const { return nullptr; }
  };

  /**
   * @brief Lvalue whose storage is a named, already-declared symbol.
   */
  // this lvalue refers to a symbol
  class Symbol : public LValue {
    const symbol::Symbol& symbol_;

  public:
    /**
     * @brief Constructs an lvalue view of a symbol-table entry.
     * @param symbol Symbol this lvalue refers to.
     */
    explicit Symbol(const symbol::Symbol& symbol);

    /**
     * @brief Confirms this lvalue is a Symbol.
     * @return This.
     */
    const Symbol* get_symbol() const override { return this; }

    /**
     * @brief Returns the wrapped symbol-table entry.
     * @return The symbol.
     */
    const symbol::Symbol& get() const { return symbol_; }

    /**
     * @brief Produces an independent duplicate wrapping the same underlying symbol.
     * @return The copied lvalue.
     */
    std::unique_ptr<LValue> copy() const override;
  };

  /**
   * @brief Lvalue whose storage is a raw memory or register location rather than a named symbol.
   */
  // this lvalue refers to a location
  class Reference : public LValue {
    memory::Ref ref_;

  public:
    /**
     * @brief Constructs an lvalue bound directly to a physical location.
     * @param type Static type of the lvalue.
     * @param ref Physical location this lvalue refers to.
     */
    Reference(const type::Node& type, memory::Ref ref) : LValue(type), ref_(std::move(ref)) {}

    /**
     * @brief Confirms this lvalue is a Reference.
     * @return This.
     */
    const Reference* get_ref() const override { return this; }

    /**
     * @brief Returns the wrapped raw location.
     * @return The reference.
     */
    const memory::Ref& get() const { return ref_; }

    /**
     * @brief Produces an independent duplicate with the same type and location.
     * @return The copied lvalue.
     */
    std::unique_ptr<LValue> copy() const override;
  };
}
