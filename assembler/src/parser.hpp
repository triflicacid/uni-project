#pragma once

#include "assembler_data.hpp"
#include "messages/list.hpp"
#include <unordered_set>
#include <instructions/argument.hpp>
#include <instructions/instruction.hpp>

/** (v_int, v_dbl, is_dbl) */
using AddBytesFunction = std::function<void(unsigned long long, double, bool)>;

namespace assembler::parser {
  /** List of valid section names. */
  extern std::unordered_set<std::string> valid_sections;

  /** Parse lines into chunks. */
  void parse(Data &data, message::List &msgs);

  /** Parse a given instruction given options (mnemonic minus base) and arguments. Assume mnemonic exists. */
  instruction::Instruction *parse_instruction(Data &data, int line_idx, int &col, message::List &msgs, uint8_t opcode,
                                              const std::string &mnemonic,
                                              std::vector<instruction::Argument> &arguments);

  /** Parse constant "<type>: <sequence>". Create byte vector on heap. Return if success. */
  bool parse_data(const Data &data, int line_idx, int &col, message::List &msgs, std::vector<unsigned char> **bytes);

  /** Parse an argument, populate <argument>.
   * Provide argtype: one of Immediate, Register, Value, Address.
   */
  void parse_arg(const Data &data, int line_idx, int &col, message::List &msgs, instruction::Argument &argument);

  /** Map of register names to offsets. */
  extern std::map<std::string, uint8_t> register_map;

  /** Given a string, return register offset, or -1. */
  int parse_register(const std::string &string, int &index);

  /** Parse numeric literal: int or float. Return if we did find a number. */
  bool parse_number(const std::string &string, int &i, bool &is_decimal, unsigned long long &v_int, double &v_dbl);

  /** Parse character. String assumed to have started with an apostrophe, with <index> pointing after this. */
  void parse_character_literal(const Data &data, int line_idx, int &col, message::List &msgs, uint64_t &value);

  /** Given a string, add bytes to data that it represents. Parse only a single data item e.g. '42'. */
  void parse_byte_item(const Data &data, int line_idx, int &col, message::List &msgs,
                       const AddBytesFunction &add_bytes);

  /** Given a string, add bytes to data that it represents. Parse an entire data string e.g., '42 0 '\0'' */
  void parse_byte_sequence(const Data &data, int line_idx, int &col, message::List &msgs,
                           const AddBytesFunction &add_bytes);
}
