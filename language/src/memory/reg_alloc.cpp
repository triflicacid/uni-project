#include "reg_alloc.hpp"
#include "assembly/create.hpp"
#include "ast/types/int.hpp"

lang::memory::RegisterAllocationManager::RegisterAllocationManager(symbol::SymbolTable& symbols, assembly::Program& program)
: symbols_(symbols), program_(program) {
  instances_.emplace();
}

int lang::memory::RegisterAllocationManager::count_free() const {
  int count = 0;
  for (auto& object : instances_.top().regs) {
    if (object)
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

lang::memory::Ref lang::memory::RegisterAllocationManager::find(lang::symbol::Variable &symbol) {
  // check if Object is inside a register
  int offset = 0;
  for (auto& object : instances_.top().regs) {
    if (object && object->type == Object::Symbol && object->symbol.get().id() == symbol.id()) {
      return Ref(Ref::Register, offset);
    }
    offset++;
  }

  // check if Object is stored in spilled memory
  for (auto& [address, object] : memory_) {
    if (object->type == Object::Symbol && object->symbol.get().id() == symbol.id()) {
      return Ref(Ref::Memory, address);
    }
  }

  // otherwise, insert
  return insert(std::make_unique<Object>(symbol));
}

lang::memory::Ref lang::memory::RegisterAllocationManager::find(ast::expr::LiteralNode& literal) {
  // check if Object is inside a register
  int offset = 0;
  for (auto& object : instances_.top().regs) {
    if (object && object->type == Object::Literal && &object->literal.get() == &literal) {
      return Ref(Ref::Register, offset);
    }
    offset++;
  }

  // check if Object is stored in spilled memory
  for (auto& [address, object] : memory_) {
    if (object->type == Object::Literal && &object->literal.get() == &literal) {
      return Ref(Ref::Memory, address);
    }
  }

  // otherwise, insert
  return insert(std::make_unique<Object>(literal));
}

lang::memory::Ref
lang::memory::RegisterAllocationManager::find(int temporary, const ast::type::Node& datatype) {
  // check if Object is inside a register
  int offset = 0;
  for (auto& object : instances_.top().regs) {
    if (object && object->type == Object::Temporary && object->temporary_id == temporary) {
      return Ref(Ref::Register, offset);
    }
    offset++;
  }

  // check if Object is stored in spilled memory
  for (auto& [address, object] : memory_) {
    if (object->type == Object::Temporary && object->temporary_id == temporary) {
      return Ref(Ref::Memory, address);
    }
  }

  // otherwise, insert
  return insert(std::make_unique<Object>(temporary, datatype));
}

std::shared_ptr<lang::memory::Object> lang::memory::RegisterAllocationManager::evict(const Ref& location) {
  if (location.type == Ref::Register) {
    // nothing is required assembly-wise
    auto object = std::move(instances_.top().regs[location.offset - initial_register]);
    instances_.top().regs[location.offset] = nullptr;
    return object;
  }

  // adjust the stack pointer if top, otherwise do nothing
  auto object = std::move(memory_[location.offset]);
  if (size_t size = object->size(); location.offset - size == instances_.top().spill_addr) {
    instances_.top().spill_addr -= size;
  }

  // remove memory address from Object mapping
  memory_.erase(location.offset);
  return object;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::insert(std::unique_ptr<Object> object) {
  // check if we have a register free
  int i = initial_register;
  for (auto& stored_object : instances_.top().regs) {
    if (stored_object) {
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

void lang::memory::RegisterAllocationManager::insert(const Ref& location, std::unique_ptr<Object> object) {
  evict(location);

  if (location.type == Ref::Register) {
    switch (object->type) {
      case Object::Literal: { // load the immediate into the register
        const auto& literal = object->literal.get();
        if (size_t size = literal.type().size(); size == 8) {
          program_.current().add(assembly::create_load_long(location.offset, literal.get()));
        } else {
          assembly::create_load(
              location.offset,
              assembly::Arg::imm(literal.get()),
              size,
              program_.current(),
              false
            );
        }
        break;
      }
      case Object::Temporary:
        // no logic here.
        // we just need to have reserved the register, which has been done.
        break;
      case Object::Symbol: { // load the value at the symbol's offset into the register
        const auto& symbol = object->symbol.get();
        const StorageLocation& storage = symbols_.locate(symbol.id());
        std::unique_ptr<assembly::BaseArg> arg;

        // TODO what if the size exceeds one word?
        if (storage.type == StorageLocation::Stack) {
          arg = assembly::Arg::reg_indirect(constants::registers::sp, symbols_.stack().offset() - storage.offset);
        } else {
          arg = assembly::Arg::label(&storage.block.get());
        }

        int idx = program_.current().size();
        assembly::create_load(
            location.offset,
            std::move(arg),
            symbol.type().size(),
            program_.current(),
            false
        );
        program_.current()[idx].comment() << "load " << (storage.type == StorageLocation::Stack ? "stack" : "global")
          << " variable " << symbol.token().image;
        break;
      }
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
  memory_[location.offset] = std::move(object);
}
