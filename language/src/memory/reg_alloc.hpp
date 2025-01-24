#pragma once

#include <memory>
#include <stack>
#include "constants.hpp"
#include "symbol/variable.hpp"
#include "ast/expr/literal.hpp"
#include "assembly/program.hpp"
#include "symbol/table.hpp"

namespace lang::memory {
  // store total number of registers we may use
  constexpr int total_registers = constants::registers::count - constants::registers::r1;

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
    Type type;
    union {
      std::reference_wrapper<symbol::Variable> symbol;
      std::reference_wrapper<ast::expr::LiteralNode> literal;
      int temporary_id;
    };
    const ast::type::Node& datatype;

    Object(symbol::Variable& symbol) : type(Symbol), symbol(symbol), datatype(symbol.type()) {}
    Object(ast::expr::LiteralNode& literal) : type(Literal), literal(literal), datatype(literal.type()) {}
    Object(int temporary_id, const ast::type::Node& datatype) : type(Temporary), temporary_id(temporary_id), datatype(datatype) {}

    size_t size() const { return datatype.size(); }
  };

  struct Store {
    std::array<std::shared_ptr<Object>, total_registers> regs; // Objects stored in registers, may be null
    uint64_t spill_addr; // current address for memory spill
    uint64_t stack_offset; // point to stack offset when store was saved
  };

  // describe reference to an item, either a register or a memory address
  struct Ref {
    enum Type {
      Register,
      Memory
    };

    Type type;
    uint64_t offset;
  };

  // class for managing register allocation and register spills
  class RegisterAllocationManager {
    std::stack<Store> instances_;
    std::map<uint64_t, std::unique_ptr<Object>> memory_; // Objects stored in memory from memory spill, may not be null
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
    Ref find(symbol::Variable& symbol);

    // return reference to an item, insert if needed
    Ref find(ast::expr::LiteralNode& literal);

    // return reference to a temporary, insert if needed (this is why we need the type)
    Ref find(int temporary, const ast::type::Node& type);

    // evict item at the given location, return Object which was evicted
    std::shared_ptr<Object> evict(const Ref& location);

    // insert Object, assume it does not exist, return reference to it
    Ref insert(std::unique_ptr<Object> object);

    // same as insert(), but put in a specific position - location is evicted if full
    void insert(const Ref& location, std::unique_ptr<Object> object);
  };
}
