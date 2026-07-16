#pragma once

#include <cstdint>

namespace lang::memory {
  /**
   * @brief A reference to a storage location that is either a register or a memory address.
   */
  struct Ref {
    /**
     * @brief Which kind of location a Ref names.
     */
    enum Type {
      Register, ///< A register.
      Memory ///< A memory address.
    };

    Type type; ///< Whether this reference names a register or a memory address.
    uint64_t offset; ///< Register index or memory address, depending on `type`.

    /**
     * @brief Compares two references for equality by kind and offset.
     * @param other Reference to compare against.
     * @return True if both have the same type and offset.
     */
    bool operator==(const Ref& other) const
    { return type == other.type && offset == other.offset; }

    /**
     * @brief Constructs a reference to a register.
     * @param r Register index.
     * @return The register reference.
     */
    static Ref reg(uint8_t r) { return Ref(Register, r); }

    /**
     * @brief Constructs a reference to a memory address.
     * @param addr Memory address.
     * @return The memory reference.
     */
    static Ref mem(uint64_t addr) { return Ref(Memory, addr); }
  };
}
