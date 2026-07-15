#pragma once

#include "memory/ref.hpp"
#include "memory/literal.hpp"

namespace lang::type {
  class Node;
}

namespace lang::value {
  /**
   * @brief Represents a computed value's current physical location: a type paired with a memory/register reference.
   */
  // an rvalue is something which has a value
  class RValue {
    const type::Node& type_;
    memory::Ref ref_;

  public:
    /**
     * @brief Constructs an rvalue at a known location.
     * @param type Static type of the value.
     * @param ref Reference identifying the value's current location.
     */
    RValue(const type::Node& type, const memory::Ref& ref) : type_(type), ref_(ref) {}

    /**
     * @brief Produces an independent duplicate of this rvalue.
     * @return The copied rvalue.
     */
    virtual std::unique_ptr<RValue> copy() const;

    /**
     * @brief Returns the rvalue's static type.
     * @return The type.
     */
    const type::Node& type() const { return type_; }

    /**
     * @brief Returns the current physical location holding this value.
     * @return The reference.
     */
    const memory::Ref& ref() const { return ref_; }

    /**
     * @brief Updates the rvalue's recorded location in place.
     * @param ref New reference to record.
     */
    void ref(const memory::Ref& ref) { ref_ = ref; }
  };
}
