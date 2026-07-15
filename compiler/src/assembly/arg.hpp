#pragma once

#include "constants.hpp"
#include <ostream>
#include <sstream>
#include <memory>

namespace lang::assembly {
  /**
   * @brief Abstract root of every instruction-argument type, requiring subclasses to print themselves and be deep-copyable through a base pointer.
   */
  struct BaseArg {
    virtual ~BaseArg() = default;

    /**
     * @brief Renders the argument's textual assembly-syntax representation.
     * @param os Output stream to write to.
     * @return The same stream, for chaining.
     */
    virtual std::ostream& print(std::ostream& os) const = 0;

    /**
     * @brief Produces a heap-allocated deep copy of this argument.
     * @return The copied argument.
     */
    virtual std::unique_ptr<BaseArg> copy() const = 0;
  };

  /**
   * @brief Operand referencing a named textual label plus a constant offset, with a flag distinguishing address-of form from plain value form.
   */
  // an argument which references a label
  class LabelArg : public BaseArg {
    const std::string label_;
    int offset_;
    bool addr_;

  public:
    /**
     * @brief Constructs a label-reference operand.
     * @param label Name of the referenced label.
     * @param offset Constant offset added to the label.
     * @param is_addr True for address-of/indirect form ("offset(label)"), false for plain value form.
     */
    LabelArg(const std::string& label, int offset, bool is_addr) : label_(label), offset_(offset), addr_(is_addr) {}

    std::ostream& print(std::ostream &os) const override;

    std::unique_ptr<BaseArg> copy() const override;
  };

  class BasicBlock;

  /**
   * @brief Operand structurally identical to LabelArg but resolving its label lazily from a live BasicBlock at print time.
   */
  // a special form of `LabelArg` which references a BasicBlock
  // error is BasicBlock does not have a label
  class BlockReferenceArg : public BaseArg {
    const BasicBlock& block_;
    int offset_;
    bool addr_;

  public:
    /**
     * @brief Constructs a reference to a specific basic block.
     * @param block Block this operand refers to; must outlive this argument.
     * @param offset Constant offset added to the block's label.
     * @param is_addr True for address-of/indirect form, false for plain value form.
     */
    BlockReferenceArg(const BasicBlock& block, int offset, bool is_addr) : block_(block), offset_(offset), addr_(is_addr) {}

    std::ostream& print(std::ostream &os) const override;

    std::unique_ptr<BaseArg> copy() const override;
  };

  /**
   * @brief Generic operand covering the four addressing modes: immediate, register, memory address, and register-indirect.
   */
  // a generic assembly argument: imm, mem, reg, reg_indirect
  class Arg : public BaseArg {
    constants::inst::arg type_;
    uint32_t value_;

  public:
    /**
     * @brief Constructs a raw generic argument from an already-encoded mode tag and payload word.
     * @param type Addressing mode this argument represents.
     * @param value Mode-specific encoded payload.
     */
    Arg(constants::inst::arg type, uint32_t value) : type_(type), value_(value) {}

    std::ostream& print(std::ostream &os) const override;

    std::unique_ptr<BaseArg> copy() const override;

    /**
     * @brief Constructs an immediate-value operand.
     * @param x Immediate value.
     * @return The newly created argument.
     */
    // create an immediate argument
    static std::unique_ptr<Arg> imm(uint32_t x);

    /**
     * @brief Constructs a register operand.
     * @param reg Register index.
     * @return The newly created argument.
     */
    // create a register argument
    static std::unique_ptr<Arg> reg(uint8_t reg);

    /**
     * @brief Constructs a direct memory-address operand.
     * @param addr Memory address.
     * @return The newly created argument.
     */
    // create a memory address argument
    static std::unique_ptr<Arg> mem(uint32_t addr);

    /**
     * @brief Constructs a register-indirect ("offset(reg)") operand.
     * @param reg Base register index.
     * @param offset Signed byte offset from the register.
     * @return The newly created argument.
     */
    // create a register-indirect argument
    static std::unique_ptr<Arg> reg_indirect(uint8_t reg, int32_t offset = 0);

    /**
     * @brief Constructs an operand referencing a named textual label.
     * @param label Name of the label.
     * @param offset Constant offset added to the label.
     * @param is_addr True for address-of/indirect form, false for plain value form.
     * @return The newly created argument.
     */
    // create an argument to a label
    static std::unique_ptr<LabelArg> label(const std::string& label, int offset = 0, bool is_addr = false);

    /**
     * @brief Constructs an operand referencing a basic block by identity.
     * @param block Block this operand refers to.
     * @param offset Constant offset added to the block's label.
     * @param is_addr True for address-of/indirect form, false for plain value form.
     * @return The newly created argument.
     */
    // create an argument referencing a BasicBlock
    static std::unique_ptr<BlockReferenceArg> label(const assembly::BasicBlock& block, int offset = 0, bool is_addr = false);
  };

  /**
   * @brief Operand wrapping a single character, printed as a quoted assembly literal.
   */
  // an argument representing a character
  class CharArg : public BaseArg {
    char ch_;

  public:
    /**
     * @brief Constructs a character-literal operand.
     * @param ch Character to wrap.
     */
    explicit CharArg(char ch) : ch_(ch) {}

    std::ostream & print(std::ostream &os) const override;

    std::unique_ptr<BaseArg> copy() const override;
  };

  /**
   * @brief Operand wrapping an arbitrary accumulated string, printed as a double-quoted assembly literal.
   */
  // an argument representing a string (null-terminated)
  class StringArg : public BaseArg {
    std::stringstream stream_;

  public:
    StringArg() = default;

    /**
     * @brief Returns the mutable underlying buffer for callers to write string content into.
     * @return The buffer stream.
     */
    std::stringstream& get() { return stream_; }

    std::ostream& print(std::ostream &os) const override;

    std::unique_ptr<BaseArg> copy() const override;
  };
}
