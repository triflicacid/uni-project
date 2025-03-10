#include <cassert>
#include "reg_alloc.hpp"
#include "assembly/create.hpp"
#include "ast/types/int.hpp"
#include "value/future.hpp"
#include "ast/types/bool.hpp"
#include "operators/builtin.hpp"
#include "operators/builtins.hpp"

lang::memory::RegisterAllocationManager::RegisterAllocationManager(symbol::SymbolTable& symbols, assembly::Program& program)
  : symbols_(symbols), program_(program) {
  instances_.emplace_front();
}

int lang::memory::RegisterAllocationManager::count_free() const {
  int count = 0;
  for (auto& object : instances_.front().regs) {
    if (!object)
      count++;
  }
  return count;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::get_oldest() const {
  int i = initial_register;
  uint32_t oldest_ticks = 0;
  int oldest_reg = i;

  for (auto& object : instances_.front().regs) {
    if (object && object->occupied_ticks >= oldest_ticks) {
      oldest_ticks = object->occupied_ticks;
      oldest_reg = i;
    }
  }

  return Ref(Ref::Register, oldest_reg);
}

void lang::memory::RegisterAllocationManager::new_store() {
  if (!instances_.empty()) instances_.front().stack_offset = symbols_.stack().offset();
  instances_.emplace_front();
}

void lang::memory::RegisterAllocationManager::save_store(bool save_registers) {
  if (!instances_.empty() && save_registers) {
    // if asked, save any occupied registers to the stack
    int i = initial_register;
    for (auto& object : instances_.front().regs) {
      if (object && object->required) {
        // create necessary space on the stack
        size_t bytes = object->size();
        symbols_.stack().push(bytes);

        program_.current().back().comment() << "save $" << constants::registers::to_string($reg(i));

        // store data in register here
        assembly::create_store(
            i,
            assembly::Arg::reg_indirect(constants::registers::sp),
            bytes,
            program_.current()
        );
      }
      i++;
    }
  }

  // if we already have an instance, copy it
  if (instances_.empty()) {
    instances_.emplace_front();
  } else {
    instances_.front().stack_offset = symbols_.stack().offset();
//    instances_.push_front(instances_.front());
    instances_.emplace_front();
  }
}

void lang::memory::RegisterAllocationManager::destroy_store(bool restore_registers) {
  mark_all_free();
  instances_.pop_front();
  if (instances_.empty()) {
    instances_.emplace_front();
    return;
  }

  if (restore_registers) {
    // point to the top of the register cache and work backwards
    uint64_t& addr = instances_.front().stack_offset;

    auto& regs = instances_.front().regs;
    for (int i = regs.size() - 1; i >= 0; i--) {
      if (auto& object = regs[i]; object && object->required) {
        size_t bytes = object->size();
        addr += bytes;

        // store region back in the correct register
        int idx = program_.current().size();
        assembly::create_load(
            initial_register + i,
            assembly::Arg::reg_indirect(constants::registers::sp, addr),
            bytes,
            program_.current(),
            false
          );
        program_.current()[idx].comment() << "restore $" << constants::registers::to_string($reg(initial_register + i));
      }
    }
  }
}

std::optional<lang::memory::Ref> lang::memory::RegisterAllocationManager::find(const symbol::Symbol& symbol) {
  // check if Object is inside a register
  int offset = initial_register;
  for (auto& object : instances_.front().regs) {
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
    if (object.value->is_lvalue()) {
      if (auto obj_symbol = object.value->lvalue().get_symbol(); obj_symbol && obj_symbol->get().id() == symbol.id()) {
        object.required = true;
        return Ref(Ref::Memory, address);
      }
    }
  }

  // otherwise, failure
  return std::nullopt;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::find_or_insert(const symbol::Symbol& symbol) {
  // get location; return if found
  // TODO make sure the symbol is always up-to-date
//  if (auto ref = find(symbol)) {
//    return *ref;
//  }

  // otherwise, insert
  auto value = value::value();
  value->lvalue(std::make_unique<value::Symbol>(symbol));
  return insert(Object(std::move(value)));
}

std::optional<lang::memory::Ref> lang::memory::RegisterAllocationManager::find(const Literal& literal) {
  // check if Object is inside a register
  int offset = initial_register;
  for (auto& object : instances_.front().regs) {
    if (object) {
      if (auto obj_lit = object->value->get_literal(); obj_lit && obj_lit->get() == literal) {
        object->required = true;
        return Ref(Ref::Register, offset);
      }
    }

    offset++;
  }

  // check if Object is stored in spilled memory
  for (auto& [address, object] : memory_) {
    if (auto obj_lit = object.value->get_literal(); obj_lit && obj_lit->get() == literal) {
      object.required = true;
      return Ref(Ref::Memory, address);
    }
  }

  // otherwise, failure
  return std::nullopt;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::find_or_insert(const Literal& literal) {
  // get location; return if found
  if (auto ref = find(literal)) {
    return *ref;
  }

  // otherwise, insert
  return insert(Object(value::literal(literal)));
}

 lang::memory::Object& lang::memory::RegisterAllocationManager::find(const lang::memory::Ref& location) {
  if (location.type == Ref::Register) {
    // lookup register, special case for $ret
    auto& maybe = location.offset == constants::registers::ret
                  ? instances_.front().ret
                  : instances_.front().regs[location.offset - initial_register];
    assert(maybe.has_value());
    return maybe.value();
  } else {
    return memory_.at(location.offset);
  }
}

bool lang::memory::RegisterAllocationManager::in_use(const lang::memory::Ref& location) const {
  if (location.type == Ref::Register) {
    auto& maybe = location.offset == constants::registers::ret
                  ? instances_.front().ret
                  : instances_.front().regs[location.offset - initial_register];
    return maybe.has_value();
  } else {
    return memory_.find(location.offset) != memory_.end();
  }
}

const lang::memory::Object& lang::memory::RegisterAllocationManager::find(const lang::memory::Ref& location) const {
  if (location.type == Ref::Register) {
    auto& maybe = location.offset == constants::registers::ret
                  ? instances_.front().ret
                  : instances_.front().regs[location.offset - initial_register];
    assert(maybe.has_value());
    return maybe.value();
  } else {
    return memory_.at(location.offset);
  }
}


void lang::memory::RegisterAllocationManager::evict(const Ref& location) {
  // remove from allocation history
  erase_if(instances_.front().history, [&](const Ref& loc) { return location == loc; });

  if (location.type == Ref::Register) {
    // nothing is required assembly-wise
    instances_.front().regs[location.offset - initial_register] = std::nullopt;
    return;
  }

  // adjust the stack pointer if top, otherwise do nothing
  auto& object = memory_.at(location.offset);
  if (size_t size = object.size(); location.offset - size == instances_.front().spill_addr) {
    instances_.front().spill_addr -= size;
  }

  // remove memory address from Object mapping
  memory_.erase(location.offset);
}

lang::memory::Ref lang::memory::RegisterAllocationManager::insert(Object object) {
  // check if we have a register free
  // if not, use the first freed register
  int i = initial_register;
  const int free_count = count_free();
  for (auto& stored_object : instances_.front().regs) {
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
  Ref location(Ref::Memory, instances_.front().spill_addr);
  insert(location, std::move(object));
  return location;
}

void lang::memory::RegisterAllocationManager::insert(const Ref& location, Object object) {
  assert(location.type == Ref::Register);
  evict(location);
  instances_.front().history.push_front(location);

  // update rvalue
  if (object.value) object.value->rvalue(location);

  if (location.type == Ref::Register) {
    // if object is empty, do nothing
    if (!object.value);
    else if (object.value->get_literal()) { // load the immediate into the register
      const auto& literal = object.value->get_literal()->get();
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

      int idx = program_.current().size();
      switch (storage.type) {
        case StorageLocation::Block: {
          auto& type = symbol.type();
          bool dereference = !(type.get_func() || type.reference_as_ptr());
          // load address into register
          program_.current().add(assembly::create_load(
              location.offset,
              assembly::Arg::label(storage.block, 0, dereference)
          ));
          break;
        }
        case StorageLocation::Stack:
          // if treating as a pointer, return address, otherwise get value at symbol
          if (auto& type = symbol.type(); type.reference_as_ptr()) {
            program_.current().add(assembly::create_sub(
                constants::inst::datatype::u64,
                location.offset,
                constants::registers::fp,
                assembly::Arg::imm(storage.base_offset)
            ));
          } else {
            program_.current().add(assembly::create_load(
                location.offset,
                assembly::Arg::reg_indirect(constants::registers::fp, -storage.base_offset)
            ));
          }
          break;
      }

      auto& comment = program_.current()[idx].comment();
      comment << symbol.full_name() << ": ";
      symbol.type().print_code(comment);
    }

    // update our records
    instances_.front().regs[location.offset - initial_register] = std::move(object);
    return;
  }

  // TODO implement this
  throw std::runtime_error("register allocator: spilling into memory is not permitted.");

  // update our records
  memory_.insert({location.offset, std::move(object)});
}

void lang::memory::RegisterAllocationManager::update(const Ref& location, Object object) {
  if (location.type == Ref::Register) {
    if (location.offset == constants::registers::ret) {
      update_ret(std::move(object));
    } else {
      instances_.front().regs[location.offset - initial_register] = std::move(object);
    }
  } else {
    memory_.erase(location.offset);
    memory_.insert({location.offset, std::move(object)});
  }
}

void lang::memory::RegisterAllocationManager::update_ret(lang::memory::Object object) {
  instances_.front().ret = std::move(object);
}

void lang::memory::RegisterAllocationManager::update_ret(const memory::Ref& ref) {
  update_ret(find(ref));
}

void lang::memory::RegisterAllocationManager::propagate_ret() {
  if (instances_.size() < 2) return;
  instances_[1].ret = instances_.front().ret;
}

std::optional<lang::memory::Ref> lang::memory::RegisterAllocationManager::get_recent(unsigned int n) const {
  if (instances_.empty() || n >= instances_.front().history.size()) return {};
  return instances_.front().history[n];
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

lang::memory::Ref lang::memory::RegisterAllocationManager::guarantee_datatype(const lang::memory::Ref& old_ref, const lang::ast::type::Node& target) {
  assert(target.size() > 0);

  // first, we must be in a register
  Ref ref = guarantee_register(old_ref);

  // fetch the object stored here
  auto& object = find(ref);

  // do the types already match?
  if (object.value->type() == target) {
    return ref;
  }

  const ast::type::Node& original = object.value->type();

  // if Boolean, do something special
  if (target == ast::type::boolean) {
    ops::boolean_cast(program_.current(), ref.offset);
  } else if (target.get_pointer() && original.get_array()) { // array -> pointer?
    // TODO array -> pointer conversion
  } else {
    // otherwise, analyse what Object contains and update the type accordingly
    // also track if we need to actually emit any instructions
    constants::inst::datatype::dt asm_original;

    if (object.value->is_lvalue() && object.value->lvalue().get_symbol()) {
      auto& symbol = object.value->lvalue().get_symbol()->get();
      // get original datatype
      auto& type = symbol.type();
      asm_original = symbol.type().get_asm_datatype();
      // remove lvalue, we are only an rvalue now
      object = Object(value::rvalue(target, ref));
    } else if (object.value->get_literal()) {
      const Literal& literal = object.value->get_literal()->get();
      // get original datatype
      asm_original = literal.type().get_asm_datatype();
      // update Object's contents
      object.value = value::literal(Literal::get(target, literal.data()));
      object.value->rvalue(ref);
    } else {
      // can't do anything, we have no further information
      return ref;
    }

    // emit cast instructions if necessary
    ops::cast(program_.current(), ref.offset, asm_original, target.get_asm_datatype());
  }

  return ref;
}

void lang::memory::RegisterAllocationManager::mark_free(const lang::memory::Ref& ref) {
  if (ref.type == Ref::Register) {
    instances_.front().regs[ref.offset - initial_register]->required = false;
  } else {
    auto& object = memory_.at(ref.offset);
    object.required = false;
    if (size_t size = object.size(); ref.offset - size == instances_.front().spill_addr) {
      instances_.front().spill_addr -= size;
    }
  }
}

void lang::memory::RegisterAllocationManager::mark_all_free() {
  Store& instance = instances_.front();

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
  for (auto& object : instances_.front().regs) {
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

std::optional<lang::memory::Object> lang::memory::RegisterAllocationManager::save_register(uint8_t reg) const {
  auto& maybe = reg == constants::registers::ret
                ? instances_.front().ret
                : instances_.front().regs[reg - initial_register];
  if (!maybe.has_value()) return {};

  // get value saved inside the register
  const value::Value& value = *maybe->value;

  // if size is 0, do nothing
  if (value.type().size() == 0) return {};

  // emit save instructions
  symbols_.stack().push(value.type().size());
  program_.current().back().comment() << "save $" << constants::registers::to_string($reg(reg));
  program_.current().add(assembly::create_store(
      reg,
      assembly::Arg::reg_indirect(constants::registers::sp)
    ));

  return maybe;
}

void lang::memory::RegisterAllocationManager::restore_register(uint8_t reg, const lang::memory::Object& object) {
  auto& maybe = reg == constants::registers::ret
                ? instances_.front().ret
                : instances_.front().regs[reg - initial_register];

  // evict old content if necessary
  if (maybe.has_value()) {
    memory::Ref ref = memory::Ref::reg(reg);
    evict(ref);
  }

  // get the old stored value
  const value::Value& value = *object.value;

  // get value from stack
  program_.current().add(assembly::create_load(
      reg,
      assembly::Arg::reg_indirect(constants::registers::sp)
  ));
  program_.current().back().comment() << "restore $" << constants::registers::to_string($reg(reg));
  symbols_.stack().pop(value.type().size());

  // set new content
  maybe = object;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::move_ret() {
  Object object = find(Ref::reg(constants::registers::ret));
  return insert(object);
}
