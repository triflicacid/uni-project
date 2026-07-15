#pragma once

#include <memory>
#include <stack>
#include "constants.hpp"
#include "ast/leaves/literal.hpp"
#include "assembly/program.hpp"
#include "symbol/table.hpp"
#include "ref.hpp"
#include "assembly/arg.hpp"
#include "value/value.hpp"

namespace lang::memory {
  /** @brief Total number of registers the allocator may use. */
  constexpr int total_registers = constants::registers::count - constants::registers::r1;

  /** @brief Register at which the allocator's register-offset indexing starts. */
  constexpr constants::registers::reg initial_register = constants::registers::r1;

  /**
   * @brief A Value wrapped with the bookkeeping metadata the register allocator needs: an LRU age counter and a flag guarding against silent eviction.
   */
  // an object is a wrapped Value with additional metadata
  struct Object {
    std::shared_ptr<value::Value> value; ///< The wrapped value.
    uint32_t occupied_ticks = 0; ///< LRU age counter, used to pick an eviction candidate.
    bool required = true; ///< If not required, this object may be evicted at any time without consequence.

    /**
     * @brief Wraps a value in a freshly-initialized object (age zero, required).
     * @param value Value to wrap.
     */
    Object(std::shared_ptr<value::Value> value) : value(std::move(value)) {}

    /**
     * @brief Returns the byte size of the wrapped value's type.
     * @return The size in bytes.
     */
    size_t size() const { return value->type().size(); }
  };

  /**
   * @brief One nested scope's worth of register-allocation state: the register file, the $ret slot, allocation history, and stack-spill bookkeeping.
   *
   * Pure data, manipulated entirely by RegisterAllocationManager.
   */
  struct Store {
    std::array<std::optional<Object>, total_registers> regs; ///< Objects stored in registers; entries may be empty.
    std::optional<Object> ret; ///< $ret register's tracked object; only accessible by a subset of functions and generally read-only.
    std::deque<Ref> history; ///< History of allocations; front (`[0]`) is the most recent.
    uint64_t stack_offset; ///< Stack offset at the point this store was saved.
    uint64_t spill_addr; ///< Current address for memory spilling.
  };

  /**
   * @brief Manages register allocation, register-to-memory spilling, and the special $ret register across a stack of nested scopes during code generation.
   *
   * Backed by the symbol table (for stack offsets and symbol storage lookup) and the
   * output program (to emit load/store/cast instructions). Memory spilling is
   * scaffolded but not fully implemented.
   */
  // class for managing register allocation and register spills
  class RegisterAllocationManager {
    std::deque<Store> instances_; ///< Stack of nested scopes; front is the most recent (innermost).
    std::map<uint64_t, Object> memory_; ///< Objects spilled to memory, keyed by address.
    assembly::Program& program_; ///< Output program instructions are emitted into.
    symbol::SymbolTable& symbols_; ///< Symbol table used for stack offsets and storage lookups.

  public:
    /**
     * @brief Constructs the manager bound to a symbol table and output program, starting with one empty scope.
     * @param symbols Symbol table used for stack offsets and storage lookups.
     * @param program Output program to emit instructions into.
     */
    RegisterAllocationManager(symbol::SymbolTable& symbols, assembly::Program& program);

    /**
     * @brief Counts how many registers are currently unoccupied in the active scope.
     * @return Number of empty registers.
     */
    // return number of empty registers
    int count_empty() const;

    /**
     * @brief Identifies the register holding the least-recently-touched value.
     * @return Reference to the oldest occupied register.
     */
    // return reference to the oldest register
    Ref get_oldest() const;

    /**
     * @brief Pushes a brand-new, empty scope onto the allocator's stack, without saving or restoring any registers.
     */
    // create a new store instance
    // IMPORTANT: take care when adding/removing new Stores, as no state is restored by generated assembly code
    void new_store();

    /**
     * @brief Pushes a new scope, optionally emitting code to physically save every occupied and required register to the stack first.
     * @param save_registers Whether to emit save instructions for occupied, required registers.
     */
    // creates a copy of the current store, useful when certain registers are cached
    void save_store(bool save_registers);

    /**
     * @brief Tears down the current (innermost) scope, optionally restoring the registers saved by a matching save_store.
     * @param restore_registers Whether to emit instructions restoring saved registers; if false, all registers in the new front scope are cleared.
     */
    // remove the latest store
    void destroy_store(bool restore_registers);

    /**
     * @brief Emits code pushing a single register's current value onto the stack.
     * @param reg Register to save.
     * @return The saved object, or nothing if the register was empty or zero-sized.
     */
    // save the given register on the stack, return object
    // return nothing if register is empty
    std::optional<Object> save_register(uint8_t reg) const;

    /**
     * @brief Emits code popping a previously-saved value off the top of the stack back into its register.
     * @param reg Register to restore into.
     * @param object Previously saved object to restore the allocator's bookkeeping to.
     */
    // restore the given register from the stack
    // note, must be at the top of the stack
    void restore_register(uint8_t reg, const Object& object);

    /**
     * @brief Searches the current scope's registers and the spill memory for a cached value backed by a symbol.
     * @param symbol Symbol to search for.
     * @return Reference to the cached location, or nothing if not cached.
     */
    // return reference to an item if present
    std::optional<Ref> find(const symbol::Symbol& symbol);

    /**
     * @brief Guarantees a symbol's value is present in a register or memory slot, always loading a fresh copy.
     * @param symbol Symbol to load.
     * @return Reference to the loaded location.
     */
    // return reference to an item, insert if needed
    Ref find_or_insert(const symbol::Symbol& symbol);

    /**
     * @brief Searches registers then spilled memory for an already-materialized copy of a literal.
     * @param literal Literal to search for.
     * @return Reference to the cached location, or nothing if not cached.
     */
    // return reference to an item if present
    std::optional<Ref> find(const Literal& literal);

    /**
     * @brief Returns a reference to an existing cached copy of a literal, or allocates and loads a fresh one.
     * @param literal Literal to find or load.
     * @return Reference to the location holding the literal.
     */
    // return reference to an item, insert if needed
    Ref find_or_insert(const Literal& literal);

    /**
     * @brief Checks whether a reference currently designates an occupied slot.
     * @param location Reference to check.
     * @return True if occupied.
     */
    // check if the given reference is in use
    bool in_use(const Ref& location) const;

    /**
     * @brief Looks up the object occupying a specific reference, assuming it exists.
     * @param location Reference to look up.
     * @return The occupying object.
     */
    // find the given reference, assume it exists
    const lang::memory::Object& find(const Ref& location) const;

    /**
     * @brief Looks up the object occupying a specific reference, assuming it exists.
     * @param location Reference to look up.
     * @return The occupying object.
     */
    // find the given reference, assume it exists
    lang::memory::Object& find(const Ref& location);

    /**
     * @brief Forcibly removes whatever currently occupies a location, without emitting any assembly.
     * @param location Reference to evict.
     */
    // evict item at the given location
    void evict(const Ref& location);

    /**
     * @brief Evicts every cached copy (register or spilled memory) of a symbol's value.
     * @param symbol Symbol whose cached copies should be invalidated.
     */
    // evict all instances of this symbol
    // used when symbol has been updated and is now invalid
    void evict(const symbol::Symbol& symbol);

    /**
     * @brief Marks whatever occupies a location as no longer required, allowing it to be silently evicted later.
     * @param ref Reference to mark free. Tolerated if it designates nothing.
     */
    // mark object as not required -- from this point onwards, data is not guaranteed to exist
    void mark_free(const Ref& ref);

    /**
     * @brief Marks every register and every current-scope spilled object as free in one call.
     */
    // mark all objects as free that were allocated in the latest instance
    void mark_all_free();

    /**
     * @brief Places a new object into whichever register or memory slot the allocator decides is appropriate.
     * @param object Object to place.
     * @return Reference to the chosen location.
     */
    // insert Object, assume it does not exist, return reference to it
    Ref insert(Object object);

    /**
     * @brief Places an object at an exact location, evicting any prior occupant and emitting the load instruction needed to materialize its value.
     * @param location Location to place the object at.
     * @param object Object to place.
     */
    // same as insert(), but put in a specific position - location is evicted if full
    void insert(const Ref& location, Object object);

    /**
     * @brief Overwrites the object recorded at a location, without emitting any load or store instructions.
     * @param location Location to update.
     * @param object New object to record.
     */
    // like insert, but just sets the value without generating any code
    void update(const Ref& location, Object object);

    /**
     * @brief Records a new value in the current scope's $ret slot, without emitting any instructions.
     * @param object Object to record as the return value.
     */
    // update the $ret register
    void update_ret(Object object);

    /**
     * @brief Sets $ret's tracked object to a copy of whatever object currently occupies another location.
     * @param ref Location to copy from.
     */
    // set $ret equal to another location
    void update_ret(const memory::Ref& ref);

    /**
     * @brief Copies the current scope's $ret object into the enclosing scope's bookkeeping.
     */
    // propagate $ret's value to the next store
    void propagate_ret();

    /**
     * @brief Physically relocates the current value of $ret into an ordinary allocated register.
     * @return Reference to the new location.
     */
    // move $ret to another register
    Ref move_ret();

    /**
     * @brief Retrieves the nth most recently allocated reference in the current scope's history.
     * @param n Depth to look back, where 0 is the most recent allocation.
     * @return The reference, or nothing if out of range.
     */
    // get the nth most recent allocation
    // default `n=0` (i.e., most recent)
    std::optional<Ref> get_recent(unsigned int n = 0) const;

    /**
     * @brief Ensures a reference designates a register, spilling it into one if it was in memory.
     * @param ref Reference to guarantee.
     * @return The (possibly new) register reference.
     */
    // ensure `Ref` is in a register, return new reference (may be equal)
    // i.e., if in memory, insert into a register
    Ref guarantee_register(const Ref& ref);

    /**
     * @brief Ensures the value at a reference has a specific datatype, emitting a conversion if it does not.
     * @param ref Reference to the value to check/convert.
     * @param target Datatype the value must have afterward.
     * @return Reference to the (now correctly-typed) register location.
     */
    // ensure `Ref` is of the given datatype - if not, a conversion is emitted
    // note, this also guarantees the value is in a register
    Ref guarantee_datatype(const Ref& ref, const type::Node& target);

    /**
     * @brief Converts an allocator-internal reference into a concrete assembly operand.
     * @param ref Reference to resolve.
     * @param mark_free Whether to mark the reference's slot as no longer required as part of resolving it.
     * @return The resolved assembly argument.
     */
    // create assembly argument resolving a reference
    // argument: mark as free?
    std::unique_ptr<assembly::Arg> resolve_ref(const Ref& ref, bool mark_free);
  };
}
