#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <deque>
#include <memory>
#include <functional>
#include <cassert>
#include "constants.hpp"
#include "bus.hpp"
#include "debug.hpp"

namespace processor {
  /**
   * @brief Core register/memory operations of a processor: register and memory access, debug-message recording.
   *
   * Most methods should *not* be used directly (see the @ref CPU class) and only if needed.
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

    /** @brief Get every recorded debug message. @return The recorded messages, oldest first. */
    [[nodiscard]] const std::deque<std::unique_ptr<debug::Message>> &get_debug_messages() const { return debug_message; }

    /** @brief Discard every recorded debug message. */
    void clear_debug_messages() { debug_message.clear(); }

    /**
     * @brief Record a new debug message, notifying @ref on_add_debug_message if set.
     * @param m Message to record; must not be null.
     */
    void add_debug_message(std::unique_ptr<debug::Message> m) {
      assert(m != nullptr);
      debug_message.push_back(std::move(m));
      if (on_add_debug_message.has_value()) on_add_debug_message.value()(*debug_message.back());
    }

    /**
     * @brief Read a register's raw bit pattern.
     * @param r Register to read.
     * @param silent If true, suppress the register-access debug message.
     * @return The register's raw 64-bit value.
     */
    [[nodiscard]] uint64_t reg(constants::registers::reg r, bool silent = false);

    /**
     * @brief Read a register, reinterpreting its bits as `T`.
     * @tparam T Datatype to reinterpret the register's bits as.
     * @param r Register to read.
     * @param silent If true, suppress the register-access debug message.
     * @return The register's value, reinterpreted as `T`.
     */
    template<typename T>
    [[nodiscard]] T reg(constants::registers::reg r, bool silent = false) {
      uint64_t raw = reg(r, silent);
      return *(T *) &raw;
    }

    /**
     * @brief Set a register's raw bit pattern.
     * @param r Register to write.
     * @param val Raw value to write.
     * @param silent If true, suppress the register-access debug message.
     */
    void reg_set(constants::registers::reg r, uint64_t val, bool silent = false);

    /**
     * @brief Copy one register's value into another.
     * @param rd Destination register.
     * @param rs Source register.
     * @param silent If true, suppress the register-access debug messages.
     */
    void reg_copy(constants::registers::reg rd, constants::registers::reg rs, bool silent = false);

    /**
     * @brief Overwrite the upper 32 bits of a register, leaving the lower 32 bits unchanged.
     * @param r Register to write.
     * @param val Value to write into the upper 32 bits.
     */
    void reg_upper(constants::registers::reg r, uint32_t val) { *(uint32_t *) &m_regs[r] = val; }

    /**
     * @brief Load a word from memory.
     * @param addr Address to load from.
     * @param size Number of bytes to load.
     * @return The loaded word.
     */
    [[nodiscard]] uint64_t mem_load(uint64_t addr, uint8_t size);

    /**
     * @brief Store a word to memory.
     * @param addr Address to store to.
     * @param size Number of bytes to store.
     * @param data Value to store.
     */
    void mem_store(uint64_t addr, uint8_t size, uint64_t data);

    /**
     * @brief Copy a block of memory from one region to another.
     * @param source_addr Address to copy from.
     * @param dest_addr Address to copy to.
     * @param length Number of bytes to copy.
     */
    void mem_copy(uint64_t source_addr, uint64_t dest_addr, uint32_t length);

    /**
     * @brief Read a fixed-length string from the input stream into memory.
     * @param addr Address to write the string to.
     * @param length Number of characters to read.
     */
    void read_string(uint64_t addr, uint32_t length);

    /**
     * @brief Write a null-terminated string from memory to the output stream.
     * @param addr Address the string starts at.
     */
    void write_string(uint64_t addr);

    /** @brief Construct a core with stdout/stdin as the default output/input streams. */
    Core() : os(&std::cout), is(&std::cin) {}

    /** @brief Reset registers and memory to their initial state; call before use. */
    void reset();

    /** @brief Print the contents of the stack as hexadecimal bytes to @ref os. */
    void print_stack();

    /** @brief Print the contents of every register as hexadecimal to @ref os. */
    void print_registers();

    /**
     * @brief Print a region of memory as hexadecimal bytes to @ref os.
     * @param addr Address the region starts at; assumed valid.
     * @param bytes Number of bytes to print.
     */
    void print_memory(uint64_t addr, uint32_t bytes);

    /**
     * @brief Read a program image into memory starting at address 0x0.
     * @param is Stream to read the program from.
     * @param bytes Number of bytes to read.
     */
    void read(std::fstream &is, size_t bytes);
  };

  /**
   * @brief Check whether an address falls within valid memory bounds.
   * @param addr Address to check.
   * @return True if `addr` is a valid memory address.
   */
  bool check_memory(uint64_t addr);

  /**
   * @brief Check whether a register offset is valid.
   * @param off Register offset to check.
   * @return True if `off` identifies a valid register.
   */
  bool check_register(uint8_t off);
}
