#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

#include "argument.hpp"

namespace assembler::instruction {
  class Instruction;

  struct Signature {
    const std::string mnemonic;
    uint8_t opcode;
    bool expect_test; // expect conditional test?
    bool expect_datatype; // expect datatype?
    std::vector<std::deque<ArgumentType>> arguments; // list of supplied args overloads
    bool is_full_word; // expect full-word immediates?
    // custom function to intercept instruction. If called, instruction IS NOT added to instruction vector.
    // Provide index of matched overload
    void (*intercept)(std::vector<Instruction *> &instructions, Instruction *instruction, int overload_index);
  };

  /** Given mnemonic, return signature. Extract options and assign to second argument. */
  Signature *find_signature(const std::string &mnemonic, std::string &options);

  Signature *find_signature(const std::string &mnemonic);

  /** Map conditional postfix to bits. */
  extern std::unordered_map<std::string, uint8_t> conditional_postfix_map;

  /** Map data-type postfix to bits. */
  extern std::map<std::string, uint8_t> datatype_postfix_map;

  /** List of all instruction signatures. Use list over map to preserve insertion order. */
  extern std::vector<Signature> signature_list;

  class Instruction {
  public:
    const Signature *signature; // signature of instruction we are representing
    uint8_t opcode; // opcode; same as signature->opcode, but provided as it may be changed
    std::deque<Argument> args; // list of supplied arguments

  private:
    // conditional test bits, only included if signature.expect_test
    // MSB - perform test, or skip?
    uint8_t test;
    // datatype specifier, only included if signature.expect_datatype
    uint8_t datatype;

  public:
    Instruction(const Signature *signature, std::deque<Argument> arguments);

    void set_conditional_test(uint8_t mask);

    void set_datatype_specifier(uint8_t mask);

    /** Offset addresses by the given amount. */
    void offset_addresses(uint16_t offset);

    [[nodiscard]] uint64_t compile() const;

    void print() const;
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

    /** Write the given data of length bits raw. */
    void write(uint8_t length, uint64_t data);

  public:
    InstructionBuilder() : m_word(0), m_pos(0), m_next(NextArgument::None) {};

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

    /** Write argument: `<reg>` */
    void arg_reg(uint8_t reg);

    /** Write argument: immediate. */
    void arg_imm(uint32_t imm);

    /** Write argument: memory address. */
    void arg_addr(uint32_t addr);

    /** Write argument: register indirect. */
    void arg_reg_indirect(uint8_t reg, int32_t offset);
  };
}
