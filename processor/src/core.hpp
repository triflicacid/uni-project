#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <deque>
#include <memory>
#include <functional>
#include "constants.hpp"
#include "bus.hpp"
#include "debug.hpp"

namespace processor {
  /**
   * Contains core functionality of a processor.
   * Most methods should *not* be used directly (see the cpu class) and only if needed.
   * Ideally, these would be marked `protected` but are not as they are needed elsewhere.
   */
  class Core {
    std::array<uint64_t, constants::registers::count> m_regs{}; // register store
    bus m_bus{}; // connected bus to access memory
    std::deque<std::unique_ptr<debug::Message>> debug_message;

  public:
    std::ostream *os; // output stream
    std::istream *is; // input stream
    std::optional<std::function<void(const debug::Message&)>> on_add_debug_message;

    // read debug message
    [[nodiscard]] const std::deque<std::unique_ptr<debug::Message>> &get_debug_messages() const { return debug_message; }

    // remove all debug messages
    void clear_debug_messages() { debug_message.clear(); }

    // get a reference to the most recent message
    [[nodiscard]] debug::Message &get_latest_debug_message() const { return *debug_message.back(); }

    template<typename T>
    [[nodiscard]] T* get_latest_debug_message() const { return (T*)debug_message.back().get(); }

    // add a new debug message
    void add_debug_message(std::unique_ptr<debug::Message> m) {
      debug_message.push_back(std::move(m));
      if (on_add_debug_message.has_value()) on_add_debug_message.value()(*debug_message.back());
    }

    [[nodiscard]] uint64_t reg(constants::registers::reg r, bool silent = false);

    template<typename T>
    [[nodiscard]] T reg(constants::registers::reg r, bool silent = false) {
      uint64_t raw = reg(r, silent);
      return *(T *) &raw;
    }

    void reg_set(constants::registers::reg r, uint64_t val, bool silent = false);

    void reg_copy(constants::registers::reg rd, constants::registers::reg rs, bool silent = false);

    void reg_upper(constants::registers::reg r, uint32_t val) { *(uint32_t *) &m_regs[r] = val; }

    [[nodiscard]] uint64_t mem_load(uint64_t addr, uint8_t size);

    void mem_store(uint64_t addr, uint8_t size, uint64_t data);

    // read a string of size `length` from the input stream and write into memory at `addr`
    void read_string(uint64_t addr, uint32_t length);

    // write a null-terminated C-string, starting at `addr`, to the output stream
    void write_string(uint64_t addr);

    Core() : os(&std::cout), is(&std::cin) {}

    // reset's the core, please call before use
    void reset();

    // print contents of stack as hexadecimal bytes
    void print_stack();

    // print contents of all registers as hexadecimal
    void print_registers();

    // print region of memory, assume locations are valid
    void print_memory(uint64_t addr, uint32_t bytes);

    // read data from the input stream into memory (starting at address 0x0)
    void read(std::fstream &is, size_t bytes);
  };

  // check if the given address is valid
  bool check_memory(uint64_t addr);

  // check if the given register is valid
  bool check_register(uint8_t off);
}
