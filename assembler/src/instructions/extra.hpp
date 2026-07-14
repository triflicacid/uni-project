#pragma once

#include <vector>
#include <messages/list.hpp>

#include "instruction.hpp"
#include "extra.hpp"
#include "assembler_data.hpp"

/** @brief `Signature::intercept`/`Signature::parse` hooks for pseudo-instructions and instructions needing special handling. */
namespace assembler::instruction::transform {
  /**
   * @brief Intercept hook that rewrites a `reg` overload into an equivalent `reg_reg` instruction (e.g. for use with `{reg, reg_reg}`).
   * @param instructions Instruction list to append the rewritten instruction to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void
  transform_reg_reg(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                    int overload);

  /**
   * @brief Intercept hook that rewrites a `reg_val` overload into an equivalent `reg_reg_val` instruction (e.g. for use with `{reg_val, reg_reg_val}`).
   * @param instructions Instruction list to append the rewritten instruction to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void transform_reg_reg_val(std::vector<std::unique_ptr<Instruction>> &instructions,
                             std::unique_ptr<Instruction> instruction, int overload);

  /**
   * @brief Intercept hook that rewrites an instruction's last `<imm>` argument to an `<imm: 8>` (byte) argument.
   * @param instructions Instruction list to append the rewritten instruction to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void transform_last_imm_to_byte(std::vector<std::unique_ptr<Instruction>> &instructions,
                                  std::unique_ptr<Instruction> instruction, int overload);

  /**
   * @brief Intercept hook that lowers a pseudo jump-and-link instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void
  transform_jal(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                int overload);

  /**
   * @brief Intercept hook that lowers the pseudo `branch` instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void branch(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
              int overload);

  /**
   * @brief Intercept hook that lowers the pseudo `exit` instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void exit(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
            int overload);

  /**
   * @brief Intercept hook that lowers the pseudo `interrupt` instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void interrupt(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                 int overload);

  /**
   * @brief Intercept hook that lowers the pseudo "return from interrupt" instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void
  interrupt_return(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                   int overload);

  /**
   * @brief Intercept hook that lowers the pseudo `ret` instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void ret(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction, int overload);

  /**
   * @brief Intercept hook that lowers the pseudo `jump` instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void jump(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
            int overload);

  /**
   * @brief Intercept hook that lowers the pseudo "load immediate" instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void
  load_immediate(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                 int overload);

  /**
   * @brief Intercept hook that lowers the pseudo `zero` instruction into its real encoding.
   * @param instructions Instruction list to append the rewritten instruction(s) to.
   * @param instruction Original instruction; ownership is transferred.
   * @param overload Index of the argument-list overload that matched.
   */
  void zero(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
            int overload);
}

/** @brief `Signature::parse` hooks for mnemonics whose trailing options need custom parsing. */
namespace assembler::instruction::parse {
  /**
   * @brief Parse hook for `cvt(d1)2(d2)`-style conversion mnemonics, extracting the source/destination datatypes.
   * @param data Current assembly data.
   * @param loc Source location of the instruction being parsed.
   * @param instruction Instruction being built; datatype specifiers are added to it.
   * @param options Remaining option text following the mnemonic; consumed as parsed.
   * @param msgs Message list to report diagnostics to.
   */
  void
  convert(const Data &data, Location &loc, std::unique_ptr<Instruction> &instruction, std::string &options,
          message::List &msgs);
}
