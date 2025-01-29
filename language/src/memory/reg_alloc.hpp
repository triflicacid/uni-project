#pragma once

#include <memory>
#include <stack>
#include "constants.hpp"
#include "symbol/variable.hpp"
#include "ast/expr/literal.hpp"
#include "assembly/program.hpp"
#include "symbol/table.hpp"
#include "ref.hpp"
#include "assembly/arg.hpp"

namespace lang::memory {
  // store total number of registers we may use
  constexpr int total_registers = 2; //constants::registers::count - constants::registers::r1;

  // register at which the offset starts
  constexpr constants::registers::reg initial_register = constants::registers::r1;

  // describe an object we are storing
  struct Object {
    enum Type {
      Symbol,
      Literal,
      Temporary
    };

    uint32_t occupied_ticks = 0;
    bool required = true; // if not required, we may be evicted at any time without consequence
    Type type;
    union {
      std::reference_wrapper<const symbol::Variable> symbol;
      std::reference_wrapper<const class Literal> literal;
      int temporary_id;
    };
    const ast::type::Node& datatype;

    Object() = delete;
    Object(const symbol::Variable& symbol) : type(Symbol), symbol(symbol), datatype(symbol.type()) {}
    Object(const ast::expr::LiteralNode& literal) : type(Literal), literal(literal.get()), datatype(literal.type()) {}
    Object(const class Literal& literal) : type(Literal), literal(literal), datatype(literal.type()) {}
    Object(int temporary_id, const ast::type::Node& datatype) : type(Temporary), temporary_id(temporary_id), datatype(datatype) {}

    size_t size() const { return datatype.size(); }
  };

  struct Store {
    std::array<std::shared_ptr<Object>, total_registers> regs; // Objects stored in registers, may be null
    std::deque<Ref> history; // history of allocations; front = [0] = most recent
    uint64_t stack_offset; // point to stack offset when store was saved
    uint64_t spill_addr; // current address for memory spill
  };

  // class for managing register allocation and register spills
  class RegisterAllocationManager {
    std::stack<Store> instances_;
    std::map<uint64_t, std::unique_ptr<Object>> memory_; // Objects stored in memory from memory spill, may not be null
    int tmpid_ = 0; // current temporary ID
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

    // return reference to an item, insert if needed
    Ref find(const symbol::Variable& symbol);

    // return reference to an item, insert if needed
    Ref find(const Literal& literal);

    // return reference to a temporary, insert if needed (this is why we need the type)
    Ref find(int temporary, const ast::type::Node& type);

    // generate an ID for a new temporary
    int new_temporary();

    // evict item at the given location, return Object which was evicted
    std::shared_ptr<Object> evict(const Ref& location);

    // mark object as not required -- from this point onwards, data is not guaranteed to exist
    void mark_free(const Ref& ref);

    // insert Object, assume it does not exist, return reference to it
    Ref insert(std::unique_ptr<Object> object);

    // same as insert(), but put in a specific position - location is evicted if full
    void insert(const Ref& location, std::unique_ptr<Object> object);

    // get the nth most recent allocation
    // default `n=0` (i.e., most recent)
    std::optional<Ref> get_recent(unsigned int n = 0) const;

    // ensure `Ref` is in a register, return new reference (may be equal)
    // i.e., if in memory, insert into a register
    Ref guarantee_register(const Ref& ref);

    // ensure `Ref` is of the given asm datatype - if not, a conversion is emitted
    // note, this also guarantees the value is in a register
    Ref guarantee_datatype(const Ref& ref, constants::inst::datatype::dt target);

    // create assembly argument resolving a reference
    // argument: mark as free?
    std::unique_ptr<assembly::Arg> resolve_ref(const Ref& ref, bool mark_free);
  };
}
