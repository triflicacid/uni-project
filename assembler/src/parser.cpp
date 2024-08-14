#include <iostream>
#include <functional>

#include "parser.hpp"
#include "messages/error.hpp"
#include "util.hpp"

extern "C" {
#include "processor/src/constants.h"
}

namespace assembler::parser {
  void parse(Data &data, message::List &msgs) {
    // Track byte offset
    uint32_t offset = 0;

    for (int line_idx = 0; line_idx < data.lines.size(); line_idx++) {
      const auto &line = data.lines[line_idx];

      // Extract first item
      int start = 0, i = 0;
      skip_to_break(line.data, i);

      // Do we have a label?
      if (line.data[i - 1] == ':') {
        std::string label_name = line.data.substr(start, i - 1);

        // Check if valid label name
        if (!is_valid_label_name(label_name)) {
          auto *err = new class message::Error(data.file_path, line.n, start, message::ErrorType::InvalidLabel);
          err->set_message("Invalid label: '" + label_name + "'");
          msgs.add(err);
          return;
        }

        if (data.debug)
          std::cout << "[" << line_idx << ":0] LABEL DECLARATION: " << label_name << "=" << offset << "\n";

        auto label = data.labels.find(label_name);

        if (label == data.labels.end()) {
          // Create a new label
          data.labels.insert({label_name, {line.n, start, offset}});
        } else {
          // Warn user that the label already exists (error if main)
          auto level = label_name == data.main_label ? message::Level::Error : message::Level::Warning;
          auto *msg = new message::Message(level, data.file_path, line.n, start);
          msg->set_message("Re-declaration of label " + label_name);
          msgs.add(msg);

          msg = new message::Message(message::Level::Note, data.file_path, label->second.line, label->second.col);
          msg->set_message("Previously declared here");
          msgs.add(msg);

          // Exit if error
          if (level == message::Level::Error)
            return;

          // Update label's information
          label->second.line = line.n;
          label->second.col = start;
          label->second.addr = offset;
        }

        // Replace all past references with its address
        data.replace_label(label_name, offset);

        // End of input?
        if (i == line.data.size()) {
          continue;
        }

        skip_whitespace(line.data, i);
        start = i;
        skip_to_break(line.data, i);
      }

      // Interpret as an instruction mnemonic
      std::string mnemonic = line.data.substr(start, i - start);

      if (data.debug)
        std::cout << "[" << line_idx << ":" << start << "] Mnemonic " << mnemonic << "\n";

      // // .section DATA
      // if (!data.strict_sections || current_section == "data") {
      //   // Is constant specifier?
      //   std::vector<unsigned char> *bytes = nullptr;
      //
      //   if (parse_data(data, line_idx, start, msgs, &bytes)) {
      //     if (data.debug)
      //       std::cout << "\tData (" << bytes->size() << " bytes)\n";
      //
      //     // Check for errors
      //     if (msgs.has_message_of(message::Level::Error)) {
      //       delete bytes;
      //       return;
      //     }
      //
      //     // Insert into a Chunk
      //     auto chunk = new Chunk(line_idx, offset);
      //     chunk->set_data(bytes);
      //     offset += chunk->get_bytes();
      //     data.chunks.push_back(chunk);
      //
      //     continue;
      //   } else if (data.strict_sections) {
      //     auto err = new class message::Error(data.file_path, line.n, start, message::ErrorType::UnknownMnemonic);
      //     err->set_message("Unknown data-type '" + mnemonic + "' (.section data)");
      //     msgs.add(err);
      //     return;
      //   }
      // }

      // check if signature exists (i.e., mnemonic exists)
      std::string options;
      auto signature = instruction::find_signature(mnemonic, options);

      if (signature == nullptr) {
        auto err = new class message::Error(data.file_path, line.n, start, message::ErrorType::UnknownMnemonic);
        err->set_message("Unknown mnemonic '" + mnemonic + "'");
        msgs.add(err);
        return;
      }

      // structure to accumulate parsed arguments
      std::deque<instruction::Argument> arguments;

      while (i < line.data.size()) {
        skip_whitespace(line.data, i);

        // parse argument
        instruction::Argument argument(i);
        parse_arg(data, line_idx, i, msgs, argument);

        // tell user which argument it was if error was encountered
        if (msgs.has_message_of(message::Level::Error)) {
          auto msg = new message::Message(message::Level::Note, data.file_path, line.n, 0);
          msg->set_message("While parsing mnemonic " + mnemonic + ", argument " + std::to_string(arguments.size() + 1));
          msgs.add(msg);

          return;
        }

        // must end in break character
        if (i < line.data.size() && line.data[i] != ' ' && line.data[i] != ',') {
          std::string ch(1, line.data[i]);

          auto err = new class message::Error(data.file_path, line.n, i, message::ErrorType::Syntax);
          err->set_message("Expected ' ' or ',', got '" + ch + "'");
          msgs.add(err);
          return;
        }

        // add to argument list
        arguments.push_back(argument);

        if (data.debug) {
          std::cout << "\tArg: ";
          argument.print();
          std::cout << "\n";
        }

        // Skip next
        if (line.data[i] == ',')
          i++;
      }

      // parse instruction
      std::vector<instruction::Instruction *> instructions;
      bool ok = parse_instruction(data, line_idx, start, msgs, mnemonic, arguments, instructions);

      // check if error occured
      // if so, add arguments as note
      // otherwise, if instruction is empty, generate error in place
      bool was_error = msgs.has_message_of(message::Error);

      if (was_error || !ok) {
        std::stringstream stream;
        stream << (was_error ? "While parsing" : "Unknown arguments for")
            << " mnemonic " << mnemonic << " (opcode 0x" << std::hex << (int) signature->opcode << std::dec
            << ")";

        if (arguments.empty()) {
          stream << "\n";
        } else {
          stream << ", arguments\n";

          for (int j = 0; j < arguments.size(); j++) {
            stream << '\t' << j + 1 << ": ";
            arguments[j].print(stream);

            if (j < arguments.size() - 1)
              stream << '\n';
          }
        }

        auto msg = new message::Message(was_error ? message::Level::Note : message::Level::Error, data.file_path,
                                        line.n, start);
        msg->set_message(stream);
        msgs.add(msg);
        return;
      }

      // insert each instruction into a Chunk
      for (auto &instruction: instructions) {
        auto chunk = new Chunk(line_idx, offset);
        chunk->set_instruction(instruction);

        data.buffer.push_back(chunk);
        offset += chunk->get_bytes();
      }
    }

    // check if any labels left...
    for (auto chunk: data.buffer) {
      if (!chunk->is_data()) {
        auto instruction = chunk->get_instruction();

        for (auto &arg: instruction->args) {
          if (arg.is_label()) {
            auto line = data.lines[chunk->get_source_line()];

            auto err = new class message::Error(data.file_path, line.n, 0, message::ErrorType::UnknownLabel);
            err->set_message("Unresolved label reference '" + *arg.get_label() + "'");
            msgs.add(err);
            return;
          }
        }
      }
    }
  }

  bool parse_instruction(Data &data, int line_idx, int &col, message::List &msgs,
                         const std::string &mnemonic,
                         std::deque<instruction::Argument> &arguments,
                         std::vector<instruction::Instruction *> &instructions) {
    // lookup signature, create instruction from it with args probided
    std::string options;
    auto signature = instruction::find_signature(mnemonic, options);

    if (signature == nullptr) {
      return false;
    }

    auto instruction = new instruction::Instruction(signature, arguments);

    // expect datatype and conditional strings
    // options = <cond>.<datatype>
    auto dot = options.find('.');

    if (signature->expect_test) {
      std::string str = dot == std::string::npos ? options.substr(0, dot) : options;

      if (str.empty()) {
        instruction->include_test_bits();
      } else {
        auto entry = instruction::conditional_postfix_map.find(str);

        if (entry == instruction::conditional_postfix_map.end()) {
          auto err = new class message::Error(data.file_path, line_idx, col, message::ErrorType::Syntax);
          err->set_message("Unknown conditional test '" + str + "'");
          msgs.add(err);

          delete instruction;
          return false;
        }

        instruction->include_test_bits(entry->second);
      }
    } else if (!options.empty() && dot == std::string::npos) {
      auto err = new class message::Error(data.file_path, line_idx, col, message::ErrorType::Syntax);
      err->set_message("Unexpected options after " + signature->mnemonic + ": '" + options + "'");
      msgs.add(err);

      delete instruction;
      return false;
    }

    if (signature->expect_datatype) {
      if (dot == std::string::npos) {
        instruction->include_datatype_specifier(DATATYPE_U64);
      } else {
        std::string str = options.substr(dot + 1);
        auto entry = instruction::datatype_postfix_map.find(str);

        if (entry == instruction::datatype_postfix_map.end()) {
          auto err = new class message::Error(data.file_path, line_idx, col, message::ErrorType::Syntax);
          err->set_message("Unknown datatype specifier '" + str + "'");
          msgs.add(err);

          delete instruction;
          return false;
        }

        instruction->include_datatype_specifier(entry->second);
      }
    } else if (dot != std::string::npos) {
      auto err = new class message::Error(data.file_path, line_idx, col, message::ErrorType::Syntax);
      err->set_message("Unexpected dot-options after " + signature->mnemonic + ": '" + options.substr(dot) + "'");
      msgs.add(err);

      delete instruction;
      return false;
    }

    // check if number of arguments match
    if (arguments.size() != signature->arguments.size()) {
      std::stringstream stream;
      stream << "Expected " << signature->arguments.size() << " argument(s), got " << arguments.size();

      auto err = new class message::Error(data.file_path, line_idx, col, message::ErrorType::BadArguments);
      err->set_message(stream);
      msgs.add(err);

      delete instruction;
      return false;
    }

    for (int i = 0; i < arguments.size(); i++) {
      // check if argument matches signature type
      if (!instruction->args[i].type_match(signature->arguments[i])) {
        std::stringstream stream;
        stream << "Expected argument " << i + 1 << " to be " << instruction::Argument::type_to_string(
          signature->arguments[i]) << ", got " << instruction::Argument::type_to_string(arguments[i].get_type());

        auto err = new class message::Error(data.file_path, line_idx, arguments[i].get_col(),
                                            message::ErrorType::BadArguments);
        err->set_message(stream);
        msgs.add(err);

        delete instruction;
        return false;
      }
    }

    // call custom handler if supplied
    if (signature->intercept == nullptr) {
      instructions.push_back(instruction);
    } else {
      signature->intercept(instructions, instruction);
    }

    return true;
  }

  // /** Add byte sequence to new vector, cast all to integers (type #1). */
  // template<typename T>
  // std::vector<unsigned char> *add_byte_sequence(const Data &data, int line_idx, int &col, message::List &msgs) {
  //   auto bytes = new std::vector<unsigned char>();
  //
  //   parse_byte_sequence(data, line_idx, col, msgs, [bytes](long long v_int, double v_dbl, bool is_dbl) {
  //     T value = static_cast<T>(v_int);
  //
  //     for (int i = 0; i < sizeof(T); i++) {
  //       bytes->push_back((value >> (i * 8)) & 0xFF);
  //     }
  //   });
  //
  //   // If empty, add 0
  //   if (bytes->empty()) {
  //     for (int i = 0; i < sizeof(T); i++) {
  //       bytes->push_back(0);
  //     }
  //   }
  //
  //   return bytes;
  // }

  // /** Add byte sequence to new vector, cast all to floats (type #1), store an type #2 (int type of same size). */
  // template<typename T, typename S>
  // std::vector<unsigned char> *add_byte_sequence_float(const Data &data, int line_idx, int &col, message::List &msgs) {
  //   auto bytes = new std::vector<unsigned char>();
  //
  //   parse_byte_sequence(data, line_idx, col, msgs, [bytes](long long v_int, double v_dbl, bool is_dbl) {
  //     T intermediate = static_cast<T>(is_dbl ? v_dbl : v_int);
  //     S value = *(S *) &intermediate;
  //
  //     for (int i = 0; i < sizeof(S); i++) {
  //       bytes->push_back((value >> (i * 8)) & 0xFF);
  //     }
  //   });
  //
  //   // If empty, add 0
  //   if (bytes->empty()) {
  //     for (int i = 0; i < sizeof(S); i++) {
  //       bytes->push_back(0);
  //     }
  //   }
  //
  //   return bytes;
  // }

  // bool parse_data(const Data &data, int line_idx, int &col, message::List &msgs, std::vector<unsigned char> **bytes) {
  //   auto &line = data.lines[line_idx];
  //
  //   // Extract datatype
  //   int start = col;
  //   skip_non_whitespace(line.data, col);
  //   std::string datatype = line.data.substr(start, col - start);
  //
  //   std::function<void(unsigned char)> add_byte = [bytes](unsigned char byte) {
  //     (*bytes)->push_back(byte);
  //   };
  //
  //   if (datatype == "u8") {
  //     *bytes = add_byte_sequence<uint8_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "i8") {
  //     *bytes = add_byte_sequence<int8_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "u16") {
  //     *bytes = add_byte_sequence<uint16_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "i16") {
  //     *bytes = add_byte_sequence<int16_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "u32") {
  //     *bytes = add_byte_sequence<uint32_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "i32") {
  //     *bytes = add_byte_sequence<int32_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "u64") {
  //     *bytes = add_byte_sequence<uint64_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "i64") {
  //     *bytes = add_byte_sequence<int64_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "f32") {
  //     *bytes = add_byte_sequence_float<float, uint32_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   if (datatype == "f64") {
  //     *bytes = add_byte_sequence_float<double, uint64_t>(data, line_idx, col, msgs);
  //     return true;
  //   }
  //
  //   col = start;
  //   return false;
  // }

  void parse_arg(const Data &data, int line_idx, int &col, message::List &msgs, instruction::Argument &argument) {
    auto line = data.lines[line_idx];
    int start;

    // store value - this is used for both immediate and register indirect
    uint64_t value = 0;
    bool found_number = false;
    bool number_decimal = false; // was source of `value` a decimal?

    // immediate - character literal
    if (line.data[col] == '\'') {
      parse_character_literal(data, line_idx, ++col, msgs, value);

      // Any errors?
      if (msgs.has_message_of(message::Level::Error)) {
        return;
      }

      // Update argument
      argument.update(instruction::ArgumentType::Immediate, value);
      return;
    }

    // register?
    if (line.data[col] == '$') {
      start = col++;
      int reg = parse_register(line.data, col);

      // is register unknown?
      if (reg == -1) {
        auto err = new class message::Error(data.file_path, line.n, start, message::ErrorType::Syntax);
        err->set_message("Unknown register");
        msgs.add(err);
        return;
      }

      // update argument
      argument.update(instruction::ArgumentType::Register, reg);
      return;
    }

    // label?
    if (std::isalpha(line.data[col])) {
      // extract label
      start = col;
      skip_alphanum(line.data, col);
      auto label = line.data.substr(start, col - start);
      col++;

      // if already exists, substitute immediately
      auto entry = data.labels.find(label);

      if (entry == data.labels.end()) {
        argument.set_label(label);
      } else {
        argument.update(instruction::ArgumentType::Address, entry->second.addr);
      }

      return;
    }

    // if integer, parse it
    // this could be immediate, or register indirect
    if (line.data[col] == '-' || std::isdigit(line.data[col])) {
      start = col;

      // parse number
      if (parse_number(line.data, col, value, number_decimal)) {
        // mark as found... this could become a register indirect!
        found_number = true;
      } else {
        // too bad, continue to parse
        col = start;
      }
    }

    // brackets? address?
    if (line.data[col] == '(') {
      start = ++col;

      // register?
      if (line.data[col] == '$') {
        col++;

        // if constant was decimal, this is bad
        if (found_number && number_decimal) {
          std::stringstream stream;
          stream << "Offset in register-indirect cannot be a decimal! (got " << *(double *) &value << ")";

          auto err = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
          err->set_message(stream);
          msgs.add(err);
          return;
        }

        // parse register
        int reg = parse_register(line.data, col);

        // is register unknown?
        if (reg == -1) {
          auto err = new class message::Error(data.file_path, line.n, start, message::ErrorType::Syntax);
          err->set_message("Unknown register");
          msgs.add(err);
          return;
        }

        // ending bracket?
        if (line.data[col] != ')') {
          std::string ch(1, line.data[col]);

          message::Message *msg = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
          msg->set_message("Expected ')', got '" + ch + "'");
          msgs.add(msg);

          msg = new message::Message(message::Level::Note, data.file_path, line.n, start - 1);
          msg->set_message("Group opened here");
          msgs.add(msg);

          return;
        }

        col++;

        // update argument
        argument.set_reg_indirect(reg, (int32_t) value);
        return;
      }

      // if found number, MUST have been register, so this is bad
      if (found_number) {
        std::string ch(1, line.data[col]);

        auto err = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
        err->set_message("Expected '$' for register-indirect, found '" + ch + "' after '('");
        msgs.add(err);
        return;
      }

      // parse as number
      if (!parse_number(line.data, col, value, number_decimal)) {
        auto err = new class message::Error(data.file_path, line.n, start, message::ErrorType::Syntax);
        err->set_message("Expected memory address");
        msgs.add(err);
        return;
      }

      // disallow decimals
      if (number_decimal) {
        auto err = new class message::Error(data.file_path, line.n, start, message::ErrorType::Syntax);
        err->set_message("Memory address cannot be a decimal!");
        msgs.add(err);
        return;
      }

      // ending bracket?
      if (line.data[col] != ')') {
        std::string ch(1, line.data[col]);

        message::Message *msg = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
        msg->set_message("Expected ')', got '" + ch + "'");
        msgs.add(msg);

        msg = new message::Message(message::Level::Note, data.file_path, line.n, start - 1);
        msg->set_message("Group opened here");
        msgs.add(msg);

        return;
      }

      col++;

      // update argument
      argument.update(instruction::ArgumentType::Address, value);
      return;
    }

    // nothing else to parse ...
    // if found number, <imm>, else throw error
    if (found_number) {
      argument.update(number_decimal
                        ? instruction::ArgumentType::DecimalImmediate
                        : instruction::ArgumentType::Immediate, value);
      return;
    }

    std::string ch(1, line.data[col]);

    auto err = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
    err->set_message("Unexpected character '" + ch + "'");
    msgs.add(err);
  }

  std::map<std::string, uint8_t> register_map = {
    {"ip", REG_IP},
    {"sp", REG_SP},
    {"fp", REG_FP},
    {"flag", REG_FLAG},
    {"ret", REG_RET},
    {"zero", REG_ZERO}
  };

  std::string register_to_string(uint8_t offset) {
    // is a special register?
    for (auto &pair : register_map) {
      if (offset == pair.second) {
        return pair.first;
      }
    }

    // is a GPR?
    if (offset >= REG_GPR && offset < REG_PGPR) {
      return "r" + std::to_string(offset - REG_GPR + 1);
    }

    // must be an RGPR
    return "s" + std::to_string(offset - REG_PGPR + 1);
  }

  int parse_register(const std::string &s, int &i) {
    // general ourpose registers
    if (s[i] == 'r' && std::isdigit(s[i + 1])) {
      i++;
      int number = s[i++] - '0';

      if (std::isdigit(s[i])) {
        number = number * 10 + s[i++] - '0';
      }

      return REG_GPR + number - 1;
    }

    // preserved general ourpose registers
    if (s[i] == 's' && std::isdigit(s[i + 1])) {
      i++;
      int number = s[i++] - '0';
      return REG_PGPR + number - 1;
    }

    // check register map
    for (auto &pair: register_map) {
      if (starts_with(s, i, pair.first)) {
        i += (int) pair.first.size();
        return pair.second;
      }
    }

    return -1;
  }

  void parse_character_literal(const Data &data, int line_idx, int &col, message::List &msgs, uint64_t &value) {
    auto &line = data.lines[line_idx];

    // Escape character
    if (line.data[col] == '\\') {
      if (!decode_escape_seq(line.data, ++col, value)) {
        auto err = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
        err->set_message("Invalid escape sequence");
        msgs.add(err);
        return;
      }
    } else {
      value = (uint8_t) line.data[col++];
    }

    // Check for ending apostrophe
    if (line.data[col] != '\'') {
      auto err = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
      err->set_message("Expected apostrophe to terminate character literal");
      msgs.add(err);
      return;
    }

    col++;
  }

  // void parse_byte_item(const Data &data, int line_idx, int &col, message::List &msgs,
  //                      const AddBytesFunction &add_bytes) {
  //   auto &line = data.lines[line_idx];
  //
  //   // Have we a character?
  //   if (line.data[col] == '\'') {
  //     unsigned long long value;
  //     parse_character_literal(data, line_idx, ++col, msgs, value);
  //
  //     if (msgs.has_message_of(message::Level::Error)) {
  //       return;
  //     }
  //
  //     add_bytes(value, 0.0, false);
  //     return;
  //   }
  //
  //   // Have we a string?
  //   if (line.data[col] == '"') {
  //     col++;
  //
  //     while (col < line.data.size() && line.data[col] != '"') {
  //       // Escape character
  //       if (line.data[col] == '\\') {
  //         auto value = decode_escape_seq(line.data, ++col);
  //
  //         if (value == -1) {
  //           auto err = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
  //           err->set_message("Invalid escape sequence");
  //           msgs.add(err);
  //           return;
  //         } else {
  //           add_bytes(value, 0.0, false);
  //         }
  //       } else {
  //         add_bytes(line.data[col++], 0.0, false);
  //       }
  //     }
  //
  //     // Must terminate string
  //     if (line.data[col] != '"') {
  //       auto err = new class message::Error(data.file_path, line.n, col, message::ErrorType::Syntax);
  //       err->set_message("Unterminated string literal");
  //       msgs.add(err);
  //       return;
  //     }
  //
  //     col++;
  //     return;
  //   }
  //
  //   // Extract characters
  //   int start = col;
  //   skip_to_break(line.data, col);
  //   std::string sub = line.data.substr(start, col - start);
  //
  //   // Is register? Not allowed here.
  //   int j = start;
  //   int reg_offset = parse_register(line.data, j);
  //
  //   if (reg_offset != -1) {
  //     auto err = new class message::Error(data.file_path, line.n, start, message::ErrorType::Syntax);
  //     err->set_message("Register symbol disallowed here '" + sub + "'");
  //     msgs.add(err);
  //     return;
  //   }
  //
  //   // Parse as number
  //   unsigned long long number_int;
  //   double number_dbl;
  //   bool is_dbl;
  //
  //   if (parse_number(sub, is_dbl, number_int, number_dbl)) {
  //     add_bytes(number_int, number_dbl, is_dbl);
  //     return;
  //   }
  //
  //   // Must be a label, then
  //   auto label = data.labels.find(sub);
  //
  //   // New label?
  //   if (label == data.labels.end()) {
  //     // TODO allow forward-referenced labels in data?
  //
  //     message::Message *msg = new class message::Error(data.file_path, line.n, col, message::ErrorType::UnknownLabel);
  //     msg->set_message("Reference to undefined label '" + sub + "'");
  //     msgs.add(msg);
  //
  //     msg = new message::Message(message::Level::Note, data.file_path, line.n, col);
  //     msg->set_message("Labels in data cannot be forward-referenced");
  //     msgs.add(msg);
  //
  //     return;
  //   } else {
  //     add_bytes(label->second.addr, 0.0, false);
  //     return;
  //   }
  // }

  // void parse_byte_sequence(const Data &data, int line_idx, int &col, message::List &msgs,
  //                          const AddBytesFunction &add_bytes) {
  //   auto &line = data.lines[line_idx];
  //   int start = col;
  //
  //   while (true) {
  //     skip_whitespace(line.data, col);
  //
  //     if (col == line.data.size())
  //       break;
  //
  //     // Parse item
  //     parse_byte_item(data, line_idx, col, msgs, add_bytes);
  //
  //     // Any errors?
  //     if (msgs.has_message_of(message::Level::Error)) {
  //       auto msg = new message::Message(message::Level::Note, data.file_path, line.n, start);
  //       msg->set_message("Data sequence starts here");
  //       msgs.add(msg);
  //
  //       return;
  //     }
  //
  //     // Skip comma
  //     if (line.data[col] == ',')
  //       col++;
  //   }
  // }
}
