#include "reg_alloc.hpp"
#include "assembly/create.hpp"
#include "ast/types/int.hpp"

size_t lang::memory::Object::size() const {
  switch (type) {
    case Symbol:
      return symbol.get().type().size();
    case Literal:
      return literal.get().type().size();
    case Temporary:
      return 8; // unknown, assume largest TODO associate size with temporary
    default:
      return 0;
  }
}

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

void lang::memory::RegisterAllocationManager::new_store() {
  instances_.emplace();
}

void lang::memory::RegisterAllocationManager::save_store() {
  // if we already have an instance, copy it
  if (instances_.empty()) {
    instances_.emplace();
  } else {
    instances_.push(instances_.top());
  }
}

void lang::memory::RegisterAllocationManager::destroy_store() {
  instances_.pop();
  if (instances_.empty()) instances_.emplace();
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
lang::memory::RegisterAllocationManager::find(int temporary) {
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
  return insert(std::make_unique<Object>(temporary));
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
  if (size_t size = object->size(); location.offset - size == instances_.top().address) {
    instances_.top().address -= size;
  }

  // remove memory address from Object mapping
  memory_.erase(location.offset);
  return object;
}

lang::memory::Ref lang::memory::RegisterAllocationManager::insert(std::unique_ptr<Object> object) {
  // check if we have a register free
  int i = initial_register;
  for (auto& stored_object : instances_.top().regs) {
    if (!stored_object) {
      Ref location(Ref::Register, i);
      insert(location, std::move(object));
      return location;
    }
    i++;
  }

  // otherwise, spill into memory
  Ref location(Ref::Memory, instances_.top().address);
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
          auto int_type = literal.type().get_int();
          assembly::create_load(
              location.offset,
              assembly::Arg::imm(literal.get()),
              size,
              program_.current(),
              int_type && int_type->is_signed()
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
          arg = assembly::Arg::label(storage.block);
        }

        int idx = program_.current().size();
        auto int_type = symbol.type().get_int();
        assembly::create_load(
            location.offset,
            std::move(arg),
            symbol.type().size(),
            program_.current(),
            int_type && int_type->is_signed()
        );
        program_.current()[idx].comment() << "load " << (storage.type == StorageLocation::Stack ? "stack" : "global")
          << " variable " << symbol.name().image;
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
