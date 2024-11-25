#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <messages/message.hpp>
#include "constants.hpp"

#include "argument.hpp"

namespace assembler::instruction {
  struct Signature;

  /** List of all instruction signatures. Use list over map to preserve insertion order. */
  extern std::vector<Signature> signature_list;

  class Instruction {
  public:
    const Signature *signature; // signature of instruction we are representing
    uint8_t overload; // selected signature overload index, default 0
    std::deque<Argument> args; // list of supplied arguments

  private:
    // conditional test bits, only included if signature.expect_test
    // MSB - perform test, or skip?
    uint8_t test;
    // datatype specifier(s), only included if signature.expect_datatype
    std::vector<constants::inst::datatype::dt> datatypes;

  public:
    Instruction(const Signature *signature, std::deque<Argument> arguments);

    void set_conditional_test(constants::cmp::flag mask);

    void add_datatype_specifier(constants::inst::datatype::dt mask);

    /** Offset addresses by the given amount. */
    void offset_addresses(uint16_t offset);

    [[nodiscard]] uint64_t compile() const;

    void debug_print(std::ostream &os) const;

    void print(std::ostream &os) const;
  };

  /** Builder class to construct an instruction word. */
  class InstructionBuilder {
  private:
    enum class NextArgument {
      None,
      AsValue,
      AsAddress
    };

    uint64_t m_word;
    uint8_t m_pos; // current bit
    NextArgument m_next;

  public:
    InstructionBuilder() : m_word(0), m_pos(0), m_next(NextArgument::None) {};

    /** Write the given data of length bits raw. */
    void write(uint8_t length, uint64_t data);

    /** Write opcode. */
    void opcode(uint8_t opcode);

    /** Get instruction word. */
    [[nodiscard]] uint64_t get() const { return m_word; }

    /** Specify next argument as <value>. */
    void next_as_value();

    /** Specify next argument as <addr>. */
    void next_as_addr();

    /** No conditional test. */
    void no_conditional_test();

    /** Add conditional test given bit mask. */
    void conditional_test(uint8_t bits);

    /** Write data-type bits. */
    void data_type(uint8_t bits);

    /** Write argument: `<reg_copy>` */
    void arg_reg(uint8_t reg);

    /** Write argument: immediate. */
    void arg_imm(uint32_t imm);

    /** Write argument: memory address. */
    void arg_addr(uint32_t addr);

    /** Write argument: register indirect. */
    void arg_reg_indirect(uint8_t reg, int32_t offset);
  };
}
