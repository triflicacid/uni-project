#pragma once

#include <cstdint>
#include <string>
#include <iostream>

namespace assembler::instruction {
  // argument type -- used for signature of type to expect
  enum class ArgumentType : uint8_t {
    Immediate, // <imm>, int
    Byte, // <imm>, byte
    DecimalImmediate, // PRIVATE. <imm>, double
    Address, // <addr> or PRIVATE <mem>
    Register, // <reg>, no indicator bits
    RegisterIndirect, // PRIVATE.
    Value, // <value>
    Label, // PRIVATE.
  };

  /** Data structure used for representing an indirect register. */
  struct ArgumentRegisterIndirect {
    uint8_t reg;
    int32_t offset;
  };

  class Argument {
  private:
    ArgumentType m_type;
    uint64_t m_data;

    void destroy();

  public:
    Argument() {
      m_type = ArgumentType::Immediate;
      m_data = 0;
    }

    Argument(ArgumentType type, uint64_t data);

    [[nodiscard]] ArgumentType get_type() const { return m_type; }

    /** Match type against given type; return if success, may update m_type. */
    bool type_match(const ArgumentType &target);

    [[nodiscard]] uint64_t get_data() const { return m_data; }

    void set_data(uint64_t data) {
      destroy();
      m_data = data;
    }

    [[nodiscard]] std::string *get_label() const { return (std::string *) m_data; };

    [[nodiscard]] ArgumentRegisterIndirect *
    get_reg_indirect() const { return (ArgumentRegisterIndirect *) m_data; };

    void update(ArgumentType type, uint64_t data);

    /** Is this argument a label? */
    [[nodiscard]] bool is_label() const { return m_type == ArgumentType::Label; }

    /** Set value to a label. */
    void set_label(const std::string &label);

    void debug_print(std::ostream &out = std::cout);

    void print(std::ostream &os = std::cout) const;

    void set_reg_indirect(uint8_t reg, int32_t offset);

    /** Check if type matches desired. Change `type` if needed. */
    static bool type_accepts(const ArgumentType &target, ArgumentType &type);

    static std::string type_to_string(const ArgumentType &type);
  };
}
