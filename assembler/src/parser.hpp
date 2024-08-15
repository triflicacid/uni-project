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

  /** Parse a given instruction given mnemonic and parsed arguments. May add multiple instructions. */
  bool parse_instruction(Data &data, int line_idx, int &col, message::List &msgs,
                                              const std::string &mnemonic,
                                              std::deque<instruction::Argument> &arguments,
                                              std::vector<instruction::Instruction *> &instructions);

  /** Parse a directive ".<directive> ...". Provide directive name, col should point to end of directive name. */
  bool parse_directive(Data &data, int line_idx, int &col, const std::string &directive, message::List &msgs);

  /** Parse a sequence of data. Give size of each element in bits: 8, 32, or 64.
   * Allocate vector on heap and fill with bytes. */
  bool parse_data(const Data &data, int line_idx, int &col, uint8_t size, message::List &msgs, std::vector<uint8_t> *&bytes);

  /** Parse an argument, populate <argument>.
   * Provide argtype: one of Immediate, Register, Value, Address.
   */
  void parse_arg(const Data &data, int line_idx, int &col, message::List &msgs, instruction::Argument &argument);

  /** Map of register names to offsets. */
  extern std::map<std::string, uint8_t> register_map;

  /** Given a string, return register offset, or -1. */
  int parse_register(const std::string &string, int &index);

  /** Given register offset, return string. */
  std::string register_to_string(uint8_t offset);

  /** Parse character. String assumed to have started with an apostrophe, with <index> pointing after this. */
  void parse_character_literal(const Data &data, int line_idx, int &col, message::List &msgs, uint64_t &value);

  /** Given a string, add bytes to data that it represents. Parse only a single data item e.g. '42'. */
  void parse_byte_item(const Data &data, int line_idx, int &col, message::List &msgs,
                       const AddBytesFunction &add_bytes);
}
