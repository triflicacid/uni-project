#pragma once

#include "assembly/arg.hpp"

namespace lang::assembly {
  class BasicBlock;
}

/** @brief Value storage and register allocation: where a value lives during compilation, and how it's committed to a permanent location. */
namespace lang::memory {
  /**
   * @brief Describes the durable, permanent storage location of a symbol: either a global tied to a basic block, or a stack-relative offset from $fp.
   *
   * Complements Ref, which describes a value's current, transient working copy
   * during register allocation. Exactly one of the union members (base_offset,
   * block) is valid to read, matching the active type.
   */
  // describe the physical storage location of a symbol
  struct StorageLocation {
    enum Type {
      Block,
      Stack, // offset `-n($fp)`
    };

    Type type;
    union {
      int base_offset;
      std::reference_wrapper<assembly::BasicBlock> block;
    };
    int offset; // offset from base

    /**
     * @brief Converts this storage description into a concrete assembly operand.
     * @param as_addr If true, suppresses the implicit load/dereference so an address (e.g. a block label) is returned rather than a loaded value.
     * @return The resolved operand.
     */
    // return assembly argument which gets value at this location
    // if is_addr, does not get value if possible (e.g., returns block label)
    std::unique_ptr<assembly::BaseArg> resolve(bool as_addr = false) const;

    /**
     * @brief Compares two storage locations for structural equality.
     * @param other Location to compare against.
     * @return True if both describe the same location (by block identity for globals, by base offset for stack locations).
     */
    bool operator==(const StorageLocation& other) const;

    /**
     * @brief Produces a new location shifted by an additional relative offset, preserving the same base.
     * @param offset Additional offset to add.
     * @return The shifted location.
     */
    // add an offset to this block
    StorageLocation operator+(int offset) const;

    /**
     * @brief Produces a new location shifted backwards by an offset, preserving the same base.
     * @param offset Offset to subtract.
     * @return The shifted location.
     */
    StorageLocation operator-(int offset) const;

    /**
     * @brief Constructs a global storage location tied to a basic block.
     * @param block Block the location is tied to.
     * @param offset Additional offset from the block's start.
     * @return The constructed location.
     */
    // location is based globally, tied to a block
    static StorageLocation global(assembly::BasicBlock& block, int offset = 0);

    /**
     * @brief Constructs a stack-relative storage location.
     * @param stack_offset Base offset from $fp.
     * @param offset Additional offset layered on top of the base.
     * @return The constructed location.
     */
    // location is `offset` on the stack
    static StorageLocation stack(int stack_offset, int offset = 0);
  };
}
