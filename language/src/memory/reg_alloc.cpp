#include <cassert>
#include "reg_alloc.hpp"
#include "assembly/create.hpp"
#include "ast/types/int.hpp"
#include "value/symbol.hpp"

lang::memory::RegisterAllocationManager::RegisterAllocationManager(symbol::SymbolTable& symbols, assembly::Program& program)
  : symbols_(symbols), program_(program) {
  instances_.emplace();
}

int lang::memory::RegisterAllocationManager::count_free() const {
  int count = 0;
  for (auto& object : instances_.top().regs) {
    if (!object)
      count++;
  }
  return count;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::get_oldest() const {
  int i = initial_register;
  uint32_t oldest_ticks = 0;
  int oldest_reg = i;

  for (auto& object : instances_.top().regs) {
    if (object && object->occupied_ticks >= oldest_ticks) {
      oldest_ticks = object->occupied_ticks;
      oldest_reg = i;
    }
  }

  return Ref(Ref::Register, oldest_reg);
}

void lang::memory::RegisterAllocationManager::new_store() {
  if (!instances_.empty()) instances_.top().stack_offset = symbols_.stack().offset();
  instances_.emplace();
}

void lang::memory::RegisterAllocationManager::save_store(bool save_registers) {
  if (!instances_.empty() && save_registers) {
    // if asked, save any occupied registers to the stack
    int i = initial_register;
    for (auto& object : instances_.top().regs) {
      if (object) {
        // create necessary space on the stack
        size_t bytes = object->size();
        uint64_t offset = symbols_.stack().offset();
        symbols_.stack().push(bytes);

        // store data in register here
        assembly::create_store(
            i,
            assembly::Arg::reg_indirect(constants::registers::sp, symbols_.stack().offset() - offset),
            bytes,
            program_.current()
        );
      }
      i++;
    }
  }

  // if we already have an instance, copy it
  if (instances_.empty()) {
    instances_.emplace();
  } else {
    instances_.top().stack_offset = symbols_.stack().offset();
    instances_.push(instances_.top());
  }
}

void lang::memory::RegisterAllocationManager::destroy_store(bool restore_registers) {
  mark_all_free();
  instances_.pop();
  if (instances_.empty()) {
    instances_.emplace();
    return;
  }

  if (restore_registers) {
    // point to the top of the register cache and work backwards
    uint64_t& addr = instances_.top().stack_offset;

    auto& regs = instances_.top().regs;;
    for (int i = regs.size() - 1; i >= 0; i--) {
      if (auto& object = regs[i]) {
        // determine offset to register save
        size_t bytes = object->size();
        addr -= bytes;

        // store region back in the correct register
        assembly::create_load(
            initial_register + i,
            assembly::Arg::reg_indirect(constants::registers::sp, addr - symbols_.stack().offset()),
            bytes,
            program_.current(),
            false
          );
      }
    }
  }
}

lang::memory::Ref lang::memory::RegisterAllocationManager::find(const lang::symbol::Symbol &symbol) {
  // check if Object is inside a register
  int offset = initial_register;
  for (auto& object : instances_.top().regs) {
    if (object && object->value->is_lvalue()) {
      if (auto obj_symbol = object->value->lvalue().get_symbol(); obj_symbol && obj_symbol->get().id() == symbol.id()) {
        object->required = true;
        return Ref(Ref::Register, offset);
      }
    }

    offset++;
  }

  // check if Object is stored in spilled memory
  for (auto& [address, object] : memory_) {
    if (auto obj_symbol = object.value->lvalue().get_symbol(); obj_symbol && obj_symbol->get().id() == symbol.id()) {
      object.required = true;
      return Ref(Ref::Memory, address);
    }
  }

  // otherwise, insert
  auto value_ptr = value::rlvalue();
  value::Value& value = *value_ptr;
  value.lvalue(std::make_unique<value::Symbol>(symbol));
  Ref ref = insert(Object(std::move(value_ptr)));
  value.rvalue(ref);
  return ref;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::find(const Literal& literal) {
  // check if Object is inside a register
  int offset = initial_register;
  for (auto& object : instances_.top().regs) {
    if (object && object->value->is_rvalue()) {
      if (auto obj_lit = object->value->rvalue().get_literal(); obj_lit && obj_lit->get().data() == literal.data()) {
        object->required = true;
        return Ref(Ref::Register, offset);
      }
    }
    offset++;
  }

  // check if Object is stored in spilled memory
  for (auto& [address, object] : memory_) {
    if (auto obj_lit = object.value->rvalue().get_literal(); obj_lit && obj_lit->get().data() == literal.data()) {
      object.required = true;
      return Ref(Ref::Memory, offset);
    }
  }

  // otherwise, insert
  auto value_ptr = value::rvalue();
  value::Value& value = *value_ptr;
  value.rvalue(std::make_unique<value::Literal>(literal, Ref::reg(0)));
  Ref ref = insert(Object(std::move(value_ptr)));
  value.rvalue().ref(ref);
  return ref;
}

const lang::memory::Object& lang::memory::RegisterAllocationManager::find(const lang::memory::Ref& location) const {
  if (location.type == Ref::Register) {
    auto& maybe = instances_.top().regs[location.offset - initial_register];
    assert(maybe.has_value());
    return maybe.value();
  } else {
    return memory_.at(location.offset);
  }
}


void lang::memory::RegisterAllocationManager::evict(const Ref& location) {
  // remove from allocation history
  erase_if(instances_.top().history, [&](const Ref& loc) { return location == loc; });

  if (location.type == Ref::Register) {
    // nothing is required assembly-wise
    instances_.top().regs[location.offset - initial_register] = std::nullopt;
    return;
  }

  // adjust the stack pointer if top, otherwise do nothing
  auto& object = memory_.at(location.offset);
  if (size_t size = object.size(); location.offset - size == instances_.top().spill_addr) {
    instances_.top().spill_addr -= size;
  }

  // remove memory address from Object mapping
  memory_.erase(location.offset);
}

lang::memory::Ref lang::memory::RegisterAllocationManager::insert(Object object) {
  // check if we have a register free
  // if not, use the first freed register
  int i = initial_register;
  const int free_count = count_free();
  for (auto& stored_object : instances_.top().regs) {
    if (stored_object && (free_count != 0 || stored_object->required)) {
      stored_object->occupied_ticks++;
    } else {
      Ref location(Ref::Register, i);
      insert(location, std::move(object));
      return location;
    }
    i++;
  }

  // otherwise, spill into memory
  Ref location(Ref::Memory, instances_.top().spill_addr);
  insert(location, std::move(object));
  return location;
}

void lang::memory::RegisterAllocationManager::insert(const Ref& location, Object object) {
  assert(location.type == Ref::Register);
  evict(location);
  instances_.top().history.push_front(location);

  if (location.type == Ref::Register) {
    if (object.value->is_rvalue() && object.value->rvalue().get_literal()) { // load the immediate into the register
      const auto& literal = object.value->rvalue().get_literal()->get();
      if (auto size = literal.type().size(); size == 8) {
        program_.current().add(assembly::create_load_long(location.offset, literal.data()));
      } else {
        program_.current().add(assembly::create_load(location.offset, assembly::Arg::imm(literal.data())));
      }

      // add comment so user knows what lit was loaded
      auto& comment = program_.current().back().comment();
      comment << literal.to_string() << ": ";
      literal.type().print_code(comment);
    } else if (object.value->is_lvalue() && object.value->lvalue().get_symbol()) { // load the value at the symbol's offset into the register
      const symbol::Symbol& symbol = object.value->lvalue().get_symbol()->get();
      const StorageLocation& storage = symbols_.locate(symbol.id())->get();
      std::unique_ptr<assembly::BaseArg> arg;

      // TODO what if the size exceeds one word?
      if (storage.type == StorageLocation::Stack) {
        arg = assembly::Arg::reg_indirect(constants::registers::sp, symbols_.stack().offset() - storage.offset);
      } else {
        arg = assembly::Arg::label(storage.block);
      }

      program_.current().add(assembly::create_load(location.offset, std::move(arg)));
      auto& comment = program_.current().back().comment();
      comment << symbol.full_name() << ": ";
      symbol.type().print_code(comment);
    }

    // update our records
    instances_.top().regs[location.offset - initial_register] = std::move(object);
    return;
  }

//  switch (object->type) {
//    case Object::Literal:
//      break;
//    case Object::Temporary:
//      break;
//    case Object::Symbol:
//      break;
//  }

  // TODO implement this
  throw std::runtime_error("register allocator: spilling into memory is not permitted.");

  // update our records
  memory_.insert({location.offset, std::move(object)});
}

void lang::memory::RegisterAllocationManager::update(const Ref& location, Object object) {
  if (location.type == Ref::Register) {
    instances_.top().regs[location.offset - initial_register] = std::move(object);
  } else {
    memory_.erase(location.offset);
    memory_.insert({location.offset, std::move(object)});
  }
}

std::optional<lang::memory::Ref> lang::memory::RegisterAllocationManager::get_recent(unsigned int n) const {
  if (instances_.empty() || n >= instances_.top().history.size()) return {};
  return instances_.top().history[n];
}

std::unique_ptr<lang::assembly::Arg> lang::memory::RegisterAllocationManager::resolve_ref(const lang::memory::Ref& ref, bool mark_free) {
  if (mark_free) this->mark_free(ref);

  if (ref.type == Ref::Register) {
    return assembly::Arg::reg(ref.offset);
  } else {
    return assembly::Arg::reg_indirect(constants::registers::sp, ref.offset - symbols_.stack().offset());
  }
}

lang::memory::Ref lang::memory::RegisterAllocationManager::guarantee_register(const lang::memory::Ref& ref) {
  // TODO implement move mechanic
  assert(ref.type == Ref::Register);
  return ref;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::guarantee_datatype(const lang::memory::Ref& old_ref, constants::inst::datatype::dt target) {
  // first, we must be in a register
  Ref ref = guarantee_register(old_ref);

  auto& object = instances_.top().regs[ref.offset - initial_register];
  assert(object.has_value());
  auto original = static_cast<constants::inst::datatype::dt>(255);

  if (object->value->is_lvalue() && object->value->lvalue().get_symbol()) {
    auto& symbol = object->value->lvalue().get_symbol()->get();
    // get original datatype
    auto &type = symbol.type();
    original = symbol.type().get_asm_datatype();
    if (target != original) {
      // remove lvalue, we are only an rvalue now
      object = Object(value::rvalue());
      object->value->rvalue(std::make_unique<value::RValue>(ast::type::from_asm_type(target), ref));
    }
  } else if (object->value->is_rvalue() && object->value->rvalue().get_literal()) {
    const Literal& literal = object->value->rvalue().get_literal()->get();
    // get original datatype
    original = literal.type().get_asm_datatype();
    if (target != original) {
      // update Object's contents
      object->value->rvalue(std::make_unique<value::Literal>(Literal::get(ast::type::from_asm_type(target), literal.data()), ref));
    }
  } else {
    // can't do anything, we have no further information
    return ref;
  }

  // emit instruction if needs be
  if (original != target) {
    program_.current().add(assembly::create_conversion(original, ref.offset, target, ref.offset));
    auto& comment = program_.current().back().comment();
    ast::type::from_asm_type(original).print_code(comment);
    comment << " -> ";
    ast::type::from_asm_type(target).print_code(comment);
  }

  return ref;
}

void lang::memory::RegisterAllocationManager::mark_free(const lang::memory::Ref& ref) {
  if (ref.type == Ref::Register) {
    instances_.top().regs[ref.offset - initial_register]->required = false;
  } else {
    auto& object = memory_.at(ref.offset);
    object.required = false;
    if (size_t size = object.size(); ref.offset - size == instances_.top().spill_addr) {
      instances_.top().spill_addr -= size;
    }
  }
}

void lang::memory::RegisterAllocationManager::mark_all_free() {
  Store& instance = instances_.top();

  // mark all registers as free
  for (auto& object : instance.regs) {
    if (object) object->required = false;
  }

  // mark all memory objects in topmost instance as free
  for (auto& [addr, object] : memory_) {
    if (addr < instance.stack_offset) continue;
    object.required = false;
  }

  // reset spill address
  instance.spill_addr = instance.stack_offset;
}

void lang::memory::RegisterAllocationManager::evict(const lang::symbol::Symbol& symbol) {
  // check if Object is inside a register
  int offset = initial_register;
  for (auto& object : instances_.top().regs) {
    if (object && object->value->is_lvalue()) {
      if (auto obj_symbol = object->value->lvalue().get_symbol(); obj_symbol && obj_symbol->get().id() == symbol.id()) {
        evict(Ref(Ref::Register, offset));
      }
    }

    offset++;
  }

  // check if Object is stored in spilled memory
  for (auto& [address, object] : memory_) {
    if (auto obj_symbol = object.value->lvalue().get_symbol(); obj_symbol && obj_symbol->get().id() == symbol.id()) {
      evict(Ref(Ref::Memory, address));
    }
  }
}
