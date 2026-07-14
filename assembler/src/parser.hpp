#pragma once

#include "assembler_data.hpp"
#include "messages/list.hpp"
#include <instructions/argument.hpp>
#include <instructions/instruction.hpp>

namespace assembler::parser {
  /**
   * @brief Write a character to a stream, substituting a placeholder at end-of-line.
   * @param os Stream to write to.
   * @param s String being read from.
   * @param i Index of the character to write.
   */
  void emit_ch(std::ostream &os, const std::string &s, int i);

  /**
   * @brief Parse every source line into compiled chunks.
   * @param data Assembly data; populated with the resulting chunks and resolved labels.
   * @param msgs Message list to report diagnostics to.
   */
  void parse(Data &data, message::List &msgs);

  /**
   * @brief Parse a single instruction from its mnemonic and already-parsed arguments, appending the resulting instruction(s).
   * @param data Current assembly data.
   * @param loc Source location of the instruction.
   * @param msgs Message list to report diagnostics to.
   * @param mnemonic Instruction mnemonic (including any test/datatype suffix).
   * @param arguments Parsed arguments supplied for this instruction.
   * @param instructions Instruction list to append the resulting instruction(s) to.
   * @return True if the instruction was parsed successfully.
   */
  bool parse_instruction(const Data &data, Location &loc, message::List &msgs,
                         const std::string &mnemonic,
                         const std::deque<instruction::Argument> &arguments,
                         std::vector<std::unique_ptr<instruction::Instruction>> &instructions);

  /**
   * @brief Parse a directive of the form `.<directive> ...`.
   * @param data Assembly data being built; updated with the directive's effect.
   * @param loc Source location of the directive; column should point just past the directive name.
   * @param line_idx Index of the current line within `data.lines`.
   * @param directive Directive name (without the leading '.').
   * @param msgs Message list to report diagnostics to.
   * @return True if the directive was parsed and applied successfully.
   */
  bool parse_directive(Data &data, Location &loc, int line_idx, const std::string &directive, message::List &msgs);

  /**
   * @brief Parse a sequence of data literals into raw bytes.
   * @param data Current assembly data.
   * @param loc Source location of the data sequence.
   * @param line_idx Index of the current line within `data.lines`.
   * @param size Size in bytes of each element: 1 (byte), 4 (half word), or 8 (word).
   * @param msgs Message list to report diagnostics to.
   * @param bytes Populated with the parsed raw bytes.
   * @return True if the data sequence was parsed successfully.
   */
  bool parse_data(const Data &data, Location &loc, int line_idx, uint8_t size, message::List &msgs, std::vector<uint8_t> &bytes);

  /**
   * @brief Parse a single instruction argument.
   * @param data Current assembly data.
   * @param loc Source location of the argument.
   * @param line_idx Index of the current line within `data.lines`.
   * @param msgs Message list to report diagnostics to.
   * @param argument Populated with the parsed argument.
   */
  void parse_arg(const Data &data, Location &loc, int line_idx, message::List &msgs, instruction::Argument &argument);

  /**
   * @brief Parse a character literal. Assumes the string started with an apostrophe, with `loc`'s index pointing just past it.
   * @param data Current assembly data.
   * @param loc Source location of the literal; index points just past the opening apostrophe.
   * @param line_idx Index of the current line within `data.lines`.
   * @param msgs Message list to report diagnostics to.
   * @param value Set to the character's value.
   */
  void parse_character_literal(const Data &data, Location &loc, int line_idx, message::List &msgs, uint64_t &value);

  /**
   * @brief Reconstruct assembly source text from compiled chunks, writing it to a stream.
   * @param data Assembly data to reconstruct from.
   * @param os Stream to write to.
   */
  void reconstruct_assembly(const Data &data, std::ostream &os);
}
