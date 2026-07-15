#pragma once

#include "printable_entity.hpp"
#include "optional_ref.hpp"
#include "value/value.hpp"
#include "shared/constants.hpp"
#include <deque>
#include <memory>

/** @brief The compiler's type system: primitive, pointer, function, and wrapper types, and the subtyping/identity rules used to check them. */
namespace lang::type {
  class IntNode;
  class FloatNode;
  class FunctionNode;
  class WrapperNode;
  class PointerNode;
  class ArrayNode;

  /** @brief Unique identifier assigned to each interned type::Node instance. */
  using TypeId = unsigned int;

  /**
   * @brief Abstract base of the type system, the type-system counterpart of ast::Node (an unrelated hierarchy despite the shared name).
   *
   * Carries a unique TypeId assigned on construction and tracked/interned by the
   * global TypeGraph. Declares pure-virtual `reference_as_ptr`, `size`, `to_label`,
   * and `get_asm_datatype`, plus down-cast helper virtuals (`get_int`, `get_float`,
   * `get_func`, `get_wrapper`, `get_pointer`, `get_array`) that default to null.
   */
  class Node : public PrintableEntity {
    TypeId id_;

  public:
    /** @brief Construct a type node, assigning it the next global TypeId. */
    Node();
    Node(const Node&) = delete;

    /** @brief Return this type's unique id. */
    TypeId id() const { return id_; }

    /**
     * @brief Return the type of the given property, if this type has one.
     * @param property Property name.
     * @return The property's type, or nothing if no such property exists.
     */
    virtual optional_ref<const Node> get_property_type(const std::string& property) const;

    /**
     * @brief Get the value of the given property, assuming `get_property_type` confirmed it exists.
     * @param ctx Compilation context.
     * @param property Property name.
     * @return The property's value, or nullptr on error.
     */
    virtual std::unique_ptr<value::Value> get_property(Context& ctx, const std::string& property) const;

    /**
     * @brief Test whether values of this type are referenced like a pointer to their first element rather than an actual stored address.
     *
     * A true pointer type stores an address explicitly; a type with this true
     * (e.g. arrays) is just raw data whose address is taken implicitly, and
     * whose values are copied wholesale when assigned.
     * @return True if this type decays to a pointer when referenced.
     */
    virtual bool reference_as_ptr() const = 0;

    /** @brief Return this as an IntNode if it is an integer type, else nullptr. */
    virtual const IntNode* get_int() const { return nullptr; }

    /** @brief Return this as a FloatNode if it is a floating-point type, else nullptr. */
    virtual const FloatNode* get_float() const { return nullptr; }

    /** @brief Return this as a FunctionNode if it is a function type, else nullptr. */
    virtual const FunctionNode* get_func() const { return nullptr; }

    /** @brief Return this as a WrapperNode if it wraps another type, else nullptr. */
    virtual const WrapperNode* get_wrapper() const { return nullptr; }

    /** @brief Return this as a PointerNode if it is a pointer type, else nullptr. */
    virtual const PointerNode* get_pointer() const { return nullptr; }

    /** @brief Return this as an ArrayNode if it is an array type, else nullptr. */
    virtual const ArrayNode* get_array() const { return nullptr; }

    /** @brief Return the size, in bytes, an instance of this type occupies. */
    virtual size_t size() const = 0;

    /** @brief Return the label representation of this type, used in generated assembly comments/output. */
    virtual std::string to_label() const = 0;

    /** @brief Return the assembly-level datatype tag representing this type. */
    virtual constants::inst::datatype::dt get_asm_datatype() const = 0;

    /**
     * @brief Compare types by identity (same TypeId).
     * @param other Type to compare against.
     * @return True if both refer to the same interned type.
     */
    bool operator==(const Node& other) const;
  };

  /**
   * @brief Return the canonical type node corresponding to an assembly-level datatype tag.
   *
   * Guaranteed that `from_asm_type(t).get_asm_datatype() == t`.
   * @param type Assembly datatype tag.
   * @return The corresponding type.
   */
  const Node& from_asm_type(constants::inst::datatype::dt type);

  /** @brief All numerical types, in order: uint8, int8, ..., uint64, int64, float32, float64. */
  extern const std::deque<std::reference_wrapper<const Node>> numerical;
}
