#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include "constants.hpp"

namespace processor::debug {
  extern bool cpu;
  extern bool args;
  extern bool mem;
  extern bool reg;
  extern bool zflag;
  extern bool conditionals;
  extern bool errs;

  void set_all(bool b);

  bool any();

  struct Message {
    enum Type {
      Cycle, // cycle number, $pc, and instruction
      Instruction,
      Argument, // argument type
      Memory, // memory access (read/write)
      Register, // register access (read/write)
      ZeroFlag, // update zero flag
      Conditional, // conditional test info
      Interrupt, // an interrupt was triggered
      Error,
    };

    Type type;

    explicit Message(Type type) : type(type) {}
  };

  struct CycleMessage : Message {
    int n = 0;
    uint64_t pc;
    uint64_t inst;

    CycleMessage(int n, uint64_t pc, uint64_t inst = 0x0) : Message(Type::Cycle), n(n), pc(pc), inst(inst) {}
  };

  struct InstructionMessage : Message {
    std::string mnemonic;
    std::stringstream message;

    explicit InstructionMessage(std::string mnemonic) : Message(Type::Instruction), mnemonic(std::move(mnemonic)) {}

    std::ostream &stream() { return message; }
  };

  struct ArgumentMessage : Message {
    constants::inst::arg arg_type;
    int n;
    std::stringstream message;
    uint64_t value = 0;

    explicit ArgumentMessage(constants::inst::arg arg_type, int n) : Message(Type::Argument), arg_type(arg_type), n(n) {}

    std::ostream &stream() { return message; }
  };

  struct MemoryMessage : Message {
    bool is_write = false;
    uint64_t address;
    uint8_t bytes;
    uint64_t value = 0;

    explicit MemoryMessage(uint64_t address, uint8_t bytes) : Message(Type::Memory), address(address), bytes(bytes) {}

    void read(uint64_t value) { is_write = false; this->value = value; }
    void write(uint64_t value) { is_write = true; this->value = value; }
  };

  struct RegisterMessage : Message {
    bool is_write = false;
    constants::registers::reg reg;
    uint64_t value = 0;

    explicit RegisterMessage(constants::registers::reg reg): Message(Type::Register), reg(reg) {}

    void read(uint64_t value) { is_write = false; this->value = value; }

    void write(uint64_t value) { is_write = true; this->value = value; }
  };

  struct ZeroFlagMessage : Message {
    constants::registers::reg reg;
    bool state;

    ZeroFlagMessage(constants::registers::reg reg, bool state): Message(Type::ZeroFlag), reg(reg), state(state) {}
  };

  struct ConditionalMessage : Message {
    constants::cmp::flag test_bits;
    bool passed = true;
    std::optional<constants::cmp::flag> flag_bits;

    explicit ConditionalMessage(constants::cmp::flag test_bits) : Message(Type::Conditional), test_bits(test_bits) {}

    void pass() { passed = true; }

    void fail(constants::cmp::flag flag_bits) {
      passed = false;
      this->flag_bits = flag_bits;
    }
  };

  struct InterruptMessage : Message {
    uint64_t isr, imr, ipc;

    InterruptMessage(uint64_t isr, uint64_t imr, uint64_t ipc) : Message(Type::Interrupt), isr(isr), imr(imr), ipc(ipc) {}
  };

  struct ErrorMessage : Message {
    std::string message;

    explicit ErrorMessage(std::string message) : Message(Type::Error), message(std::move(message)) {}
  };
}
