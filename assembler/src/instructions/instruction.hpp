#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "argument.hpp"

extern "C" {
#include "processor/src/constants.h"
}

namespace assembler::instruction {
  struct Signature {
    uint8_t opcode;
    bool expect_test;
    bool expect_datatype;
    std::vector<ArgumentType> arguments;
  };

  /** Given mnemonic, return opcode. Extract options and assign to second argument. */
  uint8_t *find_opcode(const std::string &mnemonic, std::string &options);

  /** Given mnemonic, return signature. Extract options and assign to second argument. */
  Signature *find_signature(const std::string &mnemonic, std::string &options);

  /** Map conditional postfix to bits. */
  extern std::map<std::string, uint8_t> conditional_postfix_map;

  /** Map data-type postfix to bits. */
  extern std::map<std::string, uint8_t> datatype_postfix_map;

  /** Map of standard instruction signatures. */
  extern std::map<std::string, Signature> signature_map;

  /** Map mnemonic start to opcode; does not include those in `signature_map`. */
  extern std::map<std::string, uint8_t> opcode_map;

  class Instruction {
  public:
    const std::string *mnemonic;
    uint8_t opcode;
    std::vector<Argument> args;

  private:
    // conditional test. upper 2 bits = (00 = exclude, 10 = include, no test, 11 = include, test), lower 6 bits are test bits
    // default: exclude
    uint8_t test;
    // datatype specifier. upper bit = include, lower bits = mask
    // default: exclude
    uint8_t datatype;

  public:
    Instruction(const std::string *mnemonic, uint8_t opcode, std::vector<Argument> arguments);

    /** Include conditional test bits, but skip test. */
    void include_test_bits();

    /** Include conditional test bits, provide test mask. */
    void include_test_bits(uint8_t mask);

    /** Include datatype specifier. */
    void include_datatype_specifier(uint8_t mask);

    uint64_t compile();

    void print();
  };

  /** Builder class to construct an instruction word. */
  class InstructionBuilder {
  private:
    uint64_t m_word{};
    uint8_t m_pos{}; // current bit

  public:
    /** Write opcode. */
    void opcode(uint8_t opcode);

    /** Write the given data of length bits raw. */
    void write(uint64_t data, uint8_t length);

    /** Get instruction word. */
    [[nodiscard]] uint64_t get() const { return m_word; }

    /** No conditional test. */
    void no_conditional_test();

    /** Add conditional test given bit mask. */
    void conditional_test(uint8_t bits);

    /** Write data-type bits. */
    void data_type(uint8_t bits);

    /** Write argument: `<reg>`. Write as `<value>`? */
    void arg_reg(uint8_t reg, bool as_value);

    /** Write argument: immediate. */
    void arg_imm(uint32_t imm);

    /** Write argument: memory address. */
    void arg_addr(uint32_t addr);

    /** Write argument: register indirect. */
    void arg_reg_indirect(uint8_t reg, int32_t offset);
  };
}
