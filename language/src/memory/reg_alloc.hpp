#pragma once

#include <memory>
#include <stack>
#include "constants.hpp"
#include "ast/expr/literal.hpp"
#include "assembly/program.hpp"
#include "symbol/table.hpp"
#include "ref.hpp"
#include "assembly/arg.hpp"
#include "value/value.hpp"

namespace lang::memory {
  // store total number of registers we may use
  constexpr int total_registers = constants::registers::count - constants::registers::r1;

  // register at which the offset starts
  constexpr constants::registers::reg initial_register = constants::registers::r1;

  // an object is a wrapped Value with additional metadata
  struct Object {
    std::shared_ptr<value::Value> value;
    uint32_t occupied_ticks = 0;
    bool required = true; // if not required, we may be evicted at any time without consequence

    Object(std::shared_ptr<value::Value> value) : value(std::move(value)) {}

    size_t size() const { return value->type().size(); }
  };

  struct Store {
    std::array<std::optional<Object>, total_registers> regs; // Objects stored in registers, may be null
    std::deque<Ref> history; // history of allocations; front = [0] = most recent
    uint64_t stack_offset; // point to stack offset when store was saved
    uint64_t spill_addr; // current address for memory spill
  };

  // class for managing register allocation and register spills
  class RegisterAllocationManager {
    std::stack<Store> instances_;
    std::map<uint64_t, Object> memory_; // Objects stored in memory from memory spill, may not be null
    assembly::Program& program_;
    symbol::SymbolTable& symbols_;

  public:
    RegisterAllocationManager(symbol::SymbolTable& symbols, assembly::Program& program);

    // return number of free registers
    int count_free() const;

    // return reference to the oldest register
    Ref get_oldest() const;

    // create a new store instance
    // IMPORTANT: take care when adding/removing new Stores, as no state is restored by generated assembly code
    void new_store();

    // creates a copy of the current store, useful when certain registers are cached
    void save_store(bool save_registers);

    // remove the latest store
    void destroy_store(bool restore_registers);

    // return reference to an item if present
    std::optional<Ref> find(const symbol::Symbol& symbol);

    // return reference to an item, insert if needed
    Ref find_or_insert(const symbol::Symbol& symbol);

    // return reference to an item if present
    std::optional<Ref> find(const Literal& literal);

    // return reference to an item, insert if needed
    Ref find_or_insert(const Literal& literal);

    // find the given reference
    const lang::memory::Object& find(const Ref& location) const;

    // find the given reference
    lang::memory::Object& find(const Ref& location);

    // evict item at the given location
    void evict(const Ref& location);

    // evict all instances of this symbol
    // used when symbol has been updated and is now invalid
    void evict(const symbol::Symbol& symbol);

    // mark object as not required -- from this point onwards, data is not guaranteed to exist
    void mark_free(const Ref& ref);

    // mark all objects as free that were allocated in the latest instance
    void mark_all_free();

    // insert Object, assume it does not exist, return reference to it
    Ref insert(Object object);

    // same as insert(), but put in a specific position - location is evicted if full
    void insert(const Ref& location, Object object);

    // like insert, but just sets the value without generating any code
    void update(const Ref& location, Object object);

    // get the nth most recent allocation
    // default `n=0` (i.e., most recent)
    std::optional<Ref> get_recent(unsigned int n = 0) const;

    // ensure `Ref` is in a register, return new reference (may be equal)
    // i.e., if in memory, insert into a register
    Ref guarantee_register(const Ref& ref);

    // ensure `Ref` is of the given asm datatype - if not, a conversion is emitted
    // note, this also guarantees the value is in a register
    Ref guarantee_datatype(const Ref& ref, constants::inst::datatype::dt target);

    // same as other guarantee_datatype, but handles Booleans as well
    Ref guarantee_datatype(const Ref& ref, const ast::type::Node& target);

    // like guarantee datatype, but for Boolean
    Ref guarantee_boolean(const Ref& ref);

    // create assembly argument resolving a reference
    // argument: mark as free?
    std::unique_ptr<assembly::Arg> resolve_ref(const Ref& ref, bool mark_free);
  };
}
