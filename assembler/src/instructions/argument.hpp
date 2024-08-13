#pragma once

#include <cstdint>
#include <string>
#include <iostream>

namespace assembler::instruction {
  // argument type -- used for multiple instances; argument parsing, argument exact type
  enum class ArgumentType {
    Immediate, // <imm>, no indicator bits
    ImmediateValue, // <value>, immediate
    Address, // <value>/<addr>, address
    Register, // <reg>, no indicator bits
    RegisterValue, // <value>, register
    RegisterIndirect, // <value>, register indirect
    Value, // any of <value>
    Label
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
    int m_col;

    void destroy();

  public:
    explicit Argument(int col) : m_col(col) {
      m_type = ArgumentType::Immediate;
      m_data = 0;
    }

    Argument(int col, ArgumentType type, uint64_t data);

    [[nodiscard]] ArgumentType get_type() const { return m_type; }

    /** Match type against given type; return if success. */
    bool type_match(const ArgumentType &target);

    [[nodiscard]] int get_col() const { return m_col; }

    [[nodiscard]] uint64_t get_data() const { return m_data; }

    void set_data(uint64_t data) { m_data = data; }

    /** Interpret `data` as a label. */
    [[nodiscard]] std::string *get_label() const { return (std::string *) m_data; };

    /** Interpret `data` as register offset structure. */
    [[nodiscard]] ArgumentRegisterIndirect *get_reg_indirect() const { return (ArgumentRegisterIndirect *) m_data; };

    void update(ArgumentType type, uint64_t data);

    /** Is this argument a label? */
    bool is_label();

    /** Set value to a label. */
    void set_label(const std::string &label);

    /** Transform label to constant with the given value. */
    void transform_label(uint64_t value);

    /** Print the label. */
    void print(std::ostream &out = std::cout);

    /** Set value to register indirect. */
    void set_reg_indirect(uint8_t reg, int32_t offset);

    /** Convert ArgumentType to string. */
    static std::string type_to_string(const ArgumentType &type);

    /** Check if type matches desired. Change `type` if needed e.g., `Immediate` -> `ImmediateValue` iff `target=Value`. */
    static bool type_accepts(const ArgumentType &target, ArgumentType &type);
  };
}
