#pragma once

#include <cstdint>
#include <string>
#include <iostream>

namespace assembler::instruction {
  /** @brief Kind of value an instruction argument holds, used to match it against a @ref Signature's expected argument types. */
  enum class ArgumentType : uint8_t {
    Immediate, // <imm>, int
    Byte, // <imm>, byte
    DecimalImmediate, // PRIVATE. <imm>, double
    Address, // <addr> or PRIVATE <mem>
    Register, // <reg>, no indicator bits
    RegisterIndirect, // PRIVATE.
    Value, // <value>
    Label, // PRIVATE. substituted as signature sees fit
  };

  /** @brief An indirect-register argument: a register plus a byte offset. */
  struct ArgumentRegisterIndirect {
    uint8_t reg;
    int32_t offset;
  };

  /** @brief An as-yet-unresolved label reference, to be substituted with an address once the label's location is known. */
  struct ArgumentLabel {
    std::string label;
    int offset;
    bool is_addr = false; // address if surrounded by brackets `()'
  };

  /**
   * @brief A single instruction argument, tagged by @ref ArgumentType.
   *
   * `m_data` holds either an immediate/register value directly, or (for @ref ArgumentType::RegisterIndirect
   * and @ref ArgumentType::Label) a pointer to a heap-allocated @ref ArgumentRegisterIndirect or @ref ArgumentLabel,
   * type-erased into a `uint64_t`. Which interpretation applies is determined entirely by `m_type`.
   */
  class Argument {
  private:
    ArgumentType m_type;
    uint64_t m_data;

    /** @brief Free any heap-allocated payload referenced by `m_data`, based on the current `m_type`. */
    void destroy();

  public:
    /** @brief Construct a default immediate argument with value 0. */
    Argument() {
      m_type = ArgumentType::Immediate;
      m_data = 0;
    }

    /**
     * @brief Construct an argument with an explicit type and data.
     * @param type Type of the argument.
     * @param data Raw data (immediate value, or pointer to a heap payload, depending on `type`).
     */
    Argument(ArgumentType type, uint64_t data);

    /** @brief Get the argument's type. @return The current type. */
    [[nodiscard]] ArgumentType get_type() const { return m_type; }

    /**
     * @brief Check whether this argument can satisfy a target type, adjusting its stored type if a compatible representation exists.
     * @param target Type to match against.
     * @return True if the argument matches (or was made to match) `target`.
     */
    bool type_match(const ArgumentType &target);

    /** @brief Get the raw stored data. @return Raw immediate value or payload pointer, depending on @ref get_type. */
    [[nodiscard]] uint64_t get_data() const { return m_data; }

    /**
     * @brief Replace the raw stored data, destroying any previous heap payload first.
     * @param data New raw data.
     */
    void set_data(uint64_t data) {
      destroy();
      m_data = data;
    }

    /** @brief Reinterpret the stored data as an @ref ArgumentLabel pointer. @return The label payload; only valid if @ref get_type is Label. */
    [[nodiscard]] ArgumentLabel *get_label() const { return (ArgumentLabel *) m_data; };

    /** @brief Reinterpret the stored data as an @ref ArgumentRegisterIndirect pointer. @return The register-indirect payload; only valid if @ref get_type is RegisterIndirect. */
    [[nodiscard]] ArgumentRegisterIndirect *
    get_reg_indirect() const { return (ArgumentRegisterIndirect *) m_data; };

    /**
     * @brief Replace both the type and raw data.
     * @param type New type.
     * @param data New raw data.
     */
    void update(ArgumentType type, uint64_t data);

    /** @brief Check whether this argument is currently an unresolved label. @return True if @ref get_type is Label. */
    [[nodiscard]] bool is_label() const { return m_type == ArgumentType::Label; }

    /**
     * @brief Set this argument to an unresolved label reference.
     * @param label Label name.
     * @param offset Byte offset to apply once the label is resolved.
     * @param is_addr Whether the label was written as an address (surrounded by brackets).
     */
    void set_label(const std::string &label, int offset = 0, bool is_addr = false);

    /**
     * @brief Print a verbose, debug-oriented description of this argument.
     * @param out Stream to print to.
     */
    void debug_print(std::ostream &out = std::cout);

    /**
     * @brief Print this argument in assembly syntax.
     * @param os Stream to print to.
     */
    void print(std::ostream &os = std::cout) const;

    /**
     * @brief Set this argument to a register-indirect value.
     * @param reg Register index.
     * @param offset Byte offset.
     */
    void set_reg_indirect(uint8_t reg, int32_t offset);

    /**
     * @brief Check whether a type is compatible with a target type, updating it in place if a compatible representation exists.
     * @param target Type to match against.
     * @param type Type to check and potentially update.
     * @return True if `type` matches (or was made to match) `target`.
     */
    static bool type_accepts(const ArgumentType &target, ArgumentType &type);

    /**
     * @brief Get a human-readable name for an argument type.
     * @param type Type to name.
     * @return Descriptive name.
     */
    static std::string type_to_string(const ArgumentType &type);
  };
}
