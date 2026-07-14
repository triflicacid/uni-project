#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include "constants.hpp"

/** @brief Debug-trace flags and message types describing what happens during CPU execution, used to drive step-by-step tracing/visualisation. */
namespace processor::debug {
  /** @brief Whether to emit cycle/instruction-level trace messages. */
  extern bool cpu;
  /** @brief Whether to emit argument-decoding trace messages. */
  extern bool args;
  /** @brief Whether to emit memory-access trace messages. */
  extern bool mem;
  /** @brief Whether to emit register-access trace messages. */
  extern bool reg;
  /** @brief Whether to emit zero-flag-update trace messages. */
  extern bool zflag;
  /** @brief Whether to emit conditional-test trace messages. */
  extern bool conditionals;
  /** @brief Whether to emit error trace messages. */
  extern bool errs;

  /**
   * @brief Set every debug flag to the same value.
   * @param b Value to set all flags to.
   */
  void set_all(bool b);

  /** @brief Check whether any debug flag is set. @return True if at least one flag is enabled. */
  bool any();

  /** @brief Check whether every debug flag is set. @return True if all flags are enabled. */
  bool all();

  /** @brief Base type for a single debug trace event, tagged with its @ref Type. */
  struct Message {
    /** @brief Kind of event a debug message describes. */
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

    /** @brief Construct a message of a given type. @param type Event kind. */
    explicit Message(Type type) : type(type) {}
  };

  /** @brief Trace event emitted at the start of each CPU cycle. */
  struct CycleMessage : Message {
    int n = 0;
    uint64_t pc;
    uint64_t inst;

    /**
     * @brief Construct a cycle event.
     * @param n Cycle number.
     * @param pc Program counter for this cycle.
     * @param inst Raw instruction word fetched, or 0 if not yet fetched.
     */
    CycleMessage(int n, uint64_t pc, uint64_t inst = 0x0) : Message(Type::Cycle), n(n), pc(pc), inst(inst) {}
  };

  /** @brief Trace event describing the instruction being executed and its decoded mnemonic/details. */
  struct InstructionMessage : Message {
    std::string instruction;
    std::stringstream message;

    /** @brief Construct an instruction event. @param mnemonic Instruction mnemonic. */
    explicit InstructionMessage(std::string mnemonic) : Message(Type::Instruction), instruction(std::move(mnemonic)) {}

    /** @brief Get the stream to write this event's detail text into. @return Reference to the message's text buffer. */
    std::ostream &stream() { return message; }
  };

  /** @brief Trace event describing a single decoded instruction argument. */
  struct ArgumentMessage : Message {
    constants::inst::arg arg_type;
    int n;
    std::stringstream message;
    uint64_t value = 0;

    /**
     * @brief Construct an argument event.
     * @param arg_type Kind of argument decoded.
     * @param n Index of this argument within the instruction.
     */
    explicit ArgumentMessage(constants::inst::arg arg_type, int n) : Message(Type::Argument), arg_type(arg_type), n(n) {}

    /** @brief Get the stream to write this event's detail text into. @return Reference to the message's text buffer. */
    std::ostream &stream() { return message; }
  };

  /** @brief Trace event describing a single memory read or write. */
  struct MemoryMessage : Message {
    bool is_write = false;
    uint64_t address;
    uint8_t bytes;
    uint64_t value = 0;

    /**
     * @brief Construct a memory-access event.
     * @param address Address accessed.
     * @param bytes Number of bytes accessed.
     */
    explicit MemoryMessage(uint64_t address, uint8_t bytes) : Message(Type::Memory), address(address), bytes(bytes) {}

    /** @brief Record this event as a read. @param value Value read. */
    void read(uint64_t value) { is_write = false; this->value = value; }
    /** @brief Record this event as a write. @param value Value written. */
    void write(uint64_t value) { is_write = true; this->value = value; }
  };

  /** @brief Trace event describing a single register read or write. */
  struct RegisterMessage : Message {
    bool is_write = false;
    constants::registers::reg reg;
    uint64_t value = 0;

    /** @brief Construct a register-access event. @param reg Register accessed. */
    explicit RegisterMessage(constants::registers::reg reg): Message(Type::Register), reg(reg) {}

    /** @brief Record this event as a read. @param value Value read. */
    void read(uint64_t value) { is_write = false; this->value = value; }

    /** @brief Record this event as a write. @param value Value written. */
    void write(uint64_t value) { is_write = true; this->value = value; }
  };

  /** @brief Trace event describing an update to the zero flag. */
  struct ZeroFlagMessage : Message {
    constants::registers::reg reg;
    bool state;

    /**
     * @brief Construct a zero-flag event.
     * @param reg Register the zero test was performed on.
     * @param state New state of the zero flag.
     */
    ZeroFlagMessage(constants::registers::reg reg, bool state): Message(Type::ZeroFlag), reg(reg), state(state) {}
  };

  /** @brief Trace event describing the outcome of a conditional (test) instruction. */
  struct ConditionalMessage : Message {
    constants::cmp::flag test_bits;
    bool passed = true;
    std::optional<constants::cmp::flag> flag_bits;

    /** @brief Construct a conditional event. @param test_bits Condition being tested. */
    explicit ConditionalMessage(constants::cmp::flag test_bits) : Message(Type::Conditional), test_bits(test_bits) {}

    /** @brief Record that the condition passed. */
    void pass() { passed = true; }

    /**
     * @brief Record that the condition failed.
     * @param flag_bits Actual comparison flag bits the test was checked against.
     */
    void fail(constants::cmp::flag flag_bits) {
      passed = false;
      this->flag_bits = flag_bits;
    }
  };

  /** @brief Trace event recording that an interrupt was triggered. */
  struct InterruptMessage : Message {
    uint64_t isr, imr, ipc;

    /**
     * @brief Construct an interrupt event.
     * @param isr Interrupt status register value at the time of the interrupt.
     * @param imr Interrupt mask register value at the time of the interrupt.
     * @param ipc Saved program counter to resume at after the interrupt handler returns.
     */
    InterruptMessage(uint64_t isr, uint64_t imr, uint64_t ipc) : Message(Type::Interrupt), isr(isr), imr(imr), ipc(ipc) {}
  };

  /** @brief Trace event recording an error condition. */
  struct ErrorMessage : Message {
    std::string message;

    /** @brief Construct an error event. @param message Description of the error. */
    explicit ErrorMessage(std::string message) : Message(Type::Error), message(std::move(message)) {}
  };
}
