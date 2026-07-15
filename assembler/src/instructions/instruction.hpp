#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <set>
#include <messages/message.hpp>
#include "constants.hpp"

#include "argument.hpp"

/** @brief Instruction representation: signatures, argument encoding/decoding, and the encoded Instruction objects the parser produces. */
namespace assembler::instruction {
  struct Signature;

  /** @brief Every known instruction signature. A list (not a map) is used to preserve insertion order. */
  extern std::vector<Signature> signature_list;

  /** @brief A single parsed instruction: its matched signature, selected overload, arguments, and optional test/datatype specifiers. */
  class Instruction {
  public:
    const Signature *signature; // signature of instruction we are representing
    uint8_t overload = 0; // selected signature overload index, default 0
    std::deque<Argument> args; // list of supplied arguments

  private:
    // conditional test bits, only included if signature.expect_test
    // MSB - perform test, or skip?
    uint8_t test;
    // datatype specifier(s), only included if signature.expect_datatype
    std::vector<constants::inst::datatype::dt> datatypes;

  public:
    /**
     * @brief Construct an instruction.
     * @param signature Matched signature this instruction represents.
     * @param arguments Parsed arguments supplied for this instruction.
     */
    Instruction(const Signature *signature, std::deque<Argument> arguments);

    /**
     * @brief Set the conditional test bits for this instruction.
     * @param mask Comparison condition to test before executing.
     */
    void set_conditional_test(constants::cmp::flag mask);

    /**
     * @brief Append a datatype specifier for this instruction.
     * @param mask Datatype this instruction operates on.
     */
    void add_datatype_specifier(constants::inst::datatype::dt mask);

    /**
     * @brief Shift every address-valued argument by a fixed offset.
     * @param offset Amount to add to each address.
     */
    void offset_addresses(uint16_t offset);

    /** @brief Get every label name referenced by this instruction's arguments. @return Set of referenced label names. */
    std::set<std::string> get_referenced_labels() const;

    /**
     * @brief Replace every reference to a label within this instruction's arguments with its resolved address.
     * @param label Label name to replace.
     * @param address Address to replace it with.
     * @param debug If true, print debug information about the replacement.
     */
    void replace_label(const std::string& label, uint32_t address, bool debug = false);

    /** @brief Encode this instruction into its final instruction word. @return The compiled instruction word. */
    [[nodiscard]] uint64_t compile() const;

    /**
     * @brief Print a verbose, debug-oriented description of this instruction.
     * @param os Stream to print to.
     */
    void debug_print(std::ostream &os) const;

    /**
     * @brief Print this instruction in assembly syntax.
     * @param os Stream to print to.
     */
    void print(std::ostream &os) const;
  };

  /** @brief Incrementally builds a single 64-bit instruction word, bit field by bit field. */
  class InstructionBuilder {
  private:
    /** @brief What kind of argument, if any, the next @ref write call should be interpreted as. */
    enum class NextArgument {
      None,
      AsValue,
      AsAddress
    };

    uint64_t m_word;
    uint8_t m_pos; // current bit
    NextArgument m_next;

  public:
    /** @brief Construct an empty instruction word builder. */
    InstructionBuilder() : m_word(0), m_pos(0), m_next(NextArgument::None) {};

    /**
     * @brief Write raw bits into the instruction word at the current position, advancing past them.
     * @param length Number of bits to write.
     * @param data Bits to write (only the low `length` bits are used).
     */
    void write(uint8_t length, uint64_t data);

    /**
     * @brief Write the opcode field.
     * @param opcode Opcode to write.
     */
    void opcode(uint8_t opcode);

    /** @brief Get the instruction word built so far. @return The instruction word. */
    [[nodiscard]] uint64_t get() const { return m_word; }

    /** @brief Mark the next written argument as a `<value>`. */
    void next_as_value();

    /** @brief Mark the next written argument as a `<addr>`. */
    void next_as_addr();

    /** @brief Write the "no conditional test" bits. */
    void no_conditional_test();

    /**
     * @brief Write the conditional test field.
     * @param bits Comparison condition bit mask.
     */
    void conditional_test(uint8_t bits);

    /**
     * @brief Write the datatype field.
     * @param bits Datatype bit pattern.
     */
    void data_type(uint8_t bits);

    /**
     * @brief Write a `<reg>` argument.
     * @param reg Register index.
     */
    void arg_reg(uint8_t reg);

    /**
     * @brief Write an immediate-value argument.
     * @param imm Immediate value.
     */
    void arg_imm(uint32_t imm);

    /**
     * @brief Write a memory-address argument.
     * @param addr Address.
     */
    void arg_addr(uint32_t addr);

    /**
     * @brief Write a register-indirect argument.
     * @param reg Register index.
     * @param offset Byte offset.
     */
    void arg_reg_indirect(uint8_t reg, int16_t offset);
  };
}
