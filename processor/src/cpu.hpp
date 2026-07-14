#pragma once

#include <ostream>
#include <functional>
#include "bus.hpp"
#include "constants.hpp"
#include "debug.hpp"
#include "core.hpp"

namespace processor {
  /** @brief The processor: fetch-decode-execute cycle, instruction argument decoding, flags, and interrupt handling, built on top of @ref Core. */
  class CPU : public Core {
    uint64_t addr_interrupt_handler{};

  public:
    /**
     * @brief Test whether a flag bit is set in a raw bit string.
     * @param bitstr Raw $flag-register bit pattern.
     * @param v Flag to test.
     * @return True if `v` is set in `bitstr`.
     */
    [[nodiscard]] static bool flag_test(uint64_t bitstr, constants::flag v) { return bitstr & int(v); }

    /**
     * @brief Test whether a flag is currently set.
     * @param v Flag to test.
     * @param silent If true, suppress the register-access debug message.
     * @return True if `v` is set in $flag.
     */
    [[nodiscard]] bool flag_test(constants::flag v, bool silent = false) {
      return reg(constants::registers::flag, silent) & int(v);
    }

    /**
     * @brief Set a flag bit.
     * @param v Flag to set.
     * @param silent If true, suppress the register-access debug messages.
     */
    void flag_set(constants::flag v, bool silent = false) {
      reg_set(constants::registers::flag, reg(constants::registers::flag, silent) | int(v), silent);
    }

    /**
     * @brief Clear a flag bit.
     * @param v Flag to clear.
     */
    void flag_reset(constants::flag v) {
      reg_set(constants::registers::flag, reg(constants::registers::flag) & ~int(v));
    }

    /**
     * @brief Toggle a flag bit.
     * @param v Flag to toggle.
     * @param silent If true, suppress the register-access debug messages.
     */
    void flag_toggle(constants::flag v, bool silent = false) {
      reg_set(constants::registers::flag, reg(constants::registers::flag, silent) ^ int(v), silent);
    }

    /** @brief Stop execution by clearing the `is_running` flag. */
    void halt() { flag_reset(constants::flag::is_running); }

  private:
    /**
     * @brief Push a value onto the stack, advancing $sp.
     * @tparam T Type of the value being pushed.
     * @param val Value to push.
     */
    template<typename T>
    void push(T val);

    /**
     * @brief Set the zero flag based on whether a register's value is zero.
     * @param reg Register to test.
     */
    void test_is_zero(constants::registers::reg reg);

    /**
     * @brief Decode a `<reg>` argument from a raw argument word.
     * @param data Raw argument bits.
     * @param debug_msg Argument debug message to record the decoded value into.
     * @return The decoded register.
     */
    constants::registers::reg _arg_reg(uint32_t data, std::unique_ptr<debug::ArgumentMessage>& debug_msg);

    /**
     * @brief Decode a `<addr>` argument from a raw argument word.
     * @param data Raw argument bits.
     * @param debug_msg Argument debug message to record the decoded value into.
     * @return The decoded address.
     */
    uint32_t _arg_addr(uint32_t data,  std::unique_ptr<debug::ArgumentMessage>& debug_msg);

    /**
     * @brief Decode a `<register indirect>` address argument from a raw argument word.
     * @param data Raw argument bits.
     * @param debug_msg Argument debug message to record the decoded value into.
     * @return The decoded offset.
     */
    uint32_t _arg_reg_indirect(uint32_t data,  std::unique_ptr<debug::ArgumentMessage>& debug_msg);

    /**
     * @brief Get a `<reg>` argument from an instruction word.
     * @param inst Instruction word.
     * @param pos Bit offset the argument starts at.
     * @return The decoded register.
     */
    [[nodiscard]] constants::registers::reg get_arg_reg(uint64_t inst, uint8_t pos);

    /**
     * @brief Get a `<value>` argument (immediate, register, or register-indirect) from an instruction word, fetching its value.
     * @param word Instruction word.
     * @param pos Bit offset the argument starts at.
     * @param cast_imm_double If true and the argument is a 32-bit immediate, reinterpret it as a float rather than an integer.
     * @return The argument's value.
     */
    [[nodiscard]] uint64_t get_arg_value(uint64_t word, uint8_t pos, bool cast_imm_double);

    /**
     * @brief Get a `<addr>` argument from an instruction word (the address itself, not the value stored there).
     * @param word Instruction word.
     * @param pos Bit offset the argument starts at.
     * @return The decoded address.
     */
    [[nodiscard]] uint64_t get_arg_addr(uint64_t word, uint8_t pos);

    /**
     * @brief Decode a `<reg> <reg> <value>` argument triple from an instruction, starting at `header_size + offset`.
     * @param inst Instruction word.
     * @param reg1 Set to the first decoded register.
     * @param reg2 Set to the second decoded register.
     * @param value Set to the decoded value.
     * @param offset Bit offset (past the header) the arguments start at.
     * @param is_double Passed through to @ref get_arg_value's `cast_imm_double` for the value argument.
     * @return True if all arguments were decoded successfully.
     */
    [[nodiscard]] bool
    fetch_reg_reg_val(uint64_t inst, constants::registers::reg &reg1, constants::registers::reg &reg2, uint64_t &value,
                      uint8_t offset, bool is_double);

    /** @brief Execute a load instruction. @param inst Instruction word. */
    void exec_load(uint64_t inst);

    /** @brief Execute a load-upper instruction. @param inst Instruction word. */
    void exec_load_upper(uint64_t inst);

    /** @brief Execute a store instruction. @param inst Instruction word. */
    void exec_store(uint64_t inst);

    /** @brief Execute a compare instruction. @param inst Instruction word. */
    void exec_compare(uint64_t inst);

    /** @brief Execute a datatype-convert instruction. @param inst Instruction word. */
    void exec_convert(uint64_t inst);

    /** @brief Execute a bitwise-not instruction. @param inst Instruction word. */
    void exec_not(uint64_t inst);

    /** @brief Execute a bitwise-and instruction. @param inst Instruction word. */
    void exec_and(uint64_t inst);

    /** @brief Execute a bitwise-or instruction. @param inst Instruction word. */
    void exec_or(uint64_t inst);

    /** @brief Execute a bitwise-xor instruction. @param inst Instruction word. */
    void exec_xor(uint64_t inst);

    /** @brief Execute a shift-left instruction. @param inst Instruction word. */
    void exec_shift_left(uint64_t inst);

    /** @brief Execute a shift-right instruction. @param inst Instruction word. */
    void exec_shift_right(uint64_t inst);

    /** @brief Execute a zero-extend instruction. @param inst Instruction word. */
    void exec_zero_extend(uint64_t inst);

    /** @brief Execute a sign-extend instruction. @param inst Instruction word. */
    void exec_sign_extend(uint64_t inst);

    /** @brief Execute an add instruction. @param inst Instruction word. */
    void exec_add(uint64_t inst);

    /** @brief Execute a subtract instruction. @param inst Instruction word. */
    void exec_sub(uint64_t inst);

    /** @brief Execute a multiply instruction. @param inst Instruction word. */
    void exec_mul(uint64_t inst);

    /** @brief Execute a divide instruction. @param inst Instruction word. */
    void exec_div(uint64_t inst);

    /** @brief Execute a modulo instruction. @param inst Instruction word. */
    void exec_mod(uint64_t inst);

    /** @brief Execute a jump-and-link instruction. @param inst Instruction word. */
    void exec_jal(uint64_t inst);

    /** @brief Execute a (deprecated) push instruction. @param inst Instruction word. */
    void exec_push(uint64_t inst);

    /** @brief Execute a syscall instruction, dispatching to the requested syscall handler. @param inst Instruction word. */
    void exec_syscall(uint64_t inst);

  public:
    /** @brief Construct a CPU with the default interrupt handler address. */
    CPU() : Core(), addr_interrupt_handler(constants::default_interrupt_handler) {}

    /**
     * @brief Set the address the interrupt handler starts at.
     * @param addr New interrupt handler address.
     */
    void set_interrupt_handler(uint64_t addr) { addr_interrupt_handler = addr; }

    /** @brief Read the program counter. @return Current $pc value. */
    [[nodiscard]] uint64_t read_pc() { return reg(constants::registers::pc, true); };

    /**
     * @brief Set the program counter directly; use carefully while running.
     * @param val New $pc value.
     */
    void write_pc(uint64_t val) { reg_set(constants::registers::pc, val, true); }

    /** @brief Read the return-value register. @return Current $ret value. */
    [[nodiscard]] uint64_t get_return_value() { return reg(constants::registers::ret); }

    /** @brief Check whether the CPU is still running. @return True if the `is_running` flag is set. */
    [[nodiscard]] bool is_running() { return flag_test(constants::flag::is_running); }

    /** @brief Get the current error code from $flag. @return The error code (ok if no error). */
    [[nodiscard]] constants::error::code get_error() {
      return static_cast<constants::error::code>(
          (reg(constants::registers::flag) >> constants::error::offset) & constants::error::mask);
    }

    /**
     * @brief Halt the CPU and record an error.
     * @param code Error code to set in $flag.
     * @param val Value to store in $ret.
     */
    void raise_error(constants::error::code code, uint64_t val);

    /**
     * @brief Halt the CPU and record an error, returning a caller-chosen value.
     * @param code Error code to set in $flag.
     * @param val Value to store in $ret.
     * @param ret Value to return to the caller.
     * @return `ret`, unchanged.
     */
    uint64_t raise_error(constants::error::code code, uint64_t val, uint64_t ret);

    /** @brief Reset the $flag register to its initial state. */
    void reset_flag();

    /** @brief Check whether there is a pending interrupt that is not currently being handled. @return True if an unhandled interrupt is pending. */
    [[nodiscard]] bool is_interrupt();

    /** @brief Jump to the interrupt handler. Does not check $imr or $isr. */
    void handle_interrupt();

    /** @brief Fetch the next instruction without advancing $pc. @return The fetched instruction word. */
    [[nodiscard]] uint64_t fetch();

    /**
     * @brief Decode and execute an instruction word.
     * @param inst Instruction word to execute.
     */
    void execute(uint64_t inst);

    /**
     * @brief Execute a single fetch-execute cycle.
     * @param step Current step/cycle number, for debug output only; not otherwise used.
     */
    void step(int &step);

    /** @brief Run the fetch-execute cycle repeatedly (calling @ref step) until the CPU halts. */
    void step_cycle();

    /**
     * @brief Print the current error's details, if any, to a stream.
     * @param os Stream to print to.
     * @param prefix If true, print a leading label before the error details.
     */
    void print_error(std::ostream &os, bool prefix);

    /**
     * @brief Print the current error's details, if any, to @ref Core::os.
     * @param prefix If true, print a leading label before the error details.
     */
    void print_error(bool prefix) { print_error(*os, prefix); }

    /**
     * @brief Check whether an address falls within valid memory bounds.
     * @param addr Address to check.
     * @return True if `addr` is a valid memory address.
     */
    [[nodiscard]] static bool check_memory(uint64_t addr) { return addr < dram::size; }

    /**
     * @brief Check whether a register offset is valid.
     * @param off Register offset to check.
     * @return True if `off` identifies a valid register.
     */
    [[nodiscard]] static bool check_register(uint8_t off) { return off < constants::registers::count; }
  };

  /**
   * @brief Read a compiled binary program into a CPU's memory.
   * @param cpu CPU whose memory the program is loaded into.
   * @param stream Stream to read the binary program from.
   */
  void read_binary_file(CPU &cpu, std::fstream &stream);
}