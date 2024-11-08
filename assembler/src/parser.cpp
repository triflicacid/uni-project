#include <iostream>

#include "parser.hpp"

#include <instructions/signature.hpp>
#include <bit>

#include "util.hpp"

extern "C" {
#include "processor/src/constants.h"
}

namespace assembler::parser {
  void parse(Data &data, message::List &msgs) {
    data.offset = 0;

    for (int line_idx = 0; line_idx < data.lines.size(); line_idx++) {
      const auto &line = data.lines[line_idx];

      // Extract first item
      int start = 0, i = 0;
      skip_to_break(line.data, i);

      // Do we have a directive?
      if (line.data[0] == '.') {
        std::string directive = line.data.substr(start + 1, i - 1);
        skip_whitespace(line.data, i);

        if (!parse_directive(data, line_idx, i, directive, msgs)) {
          message::Message *msg;

          if (msgs.has_message_of(message::Error)) {
            msg = new message::Message(message::Note, data.file_path, line_idx, start);
            msg->set_message("whilst parsing directive ." + directive);
          } else {
            msg = new message::Message(message::Error, data.file_path, line_idx, start);
            msg->set_message("unknown directive ." + directive);
          }

          msgs.add(msg);
          return;
        }

        continue;
      }

      // Do we have a label?
      if (line.data[i - 1] == ':') {
        std::string label_name = line.data.substr(start, i - 1);

        // Check if valid label name
        if (!is_valid_label_name(label_name)) {
          auto *err = new message::Message(message::Error, data.file_path, line.n, start);
          err->set_message("invalid label: '" + label_name + "'");
          msgs.add(err);
          return;
        }

        if (data.debug)
          std::cout << "[" << line_idx << ":0] Label: " << label_name << " = 0x" << std::hex << data.offset << std::dec << '\n';

        if (auto label = data.labels.find(label_name); label == data.labels.end()) {
          // Create a new label
          data.labels.insert({label_name, {line.n, start, data.offset}});
        } else {
          // Warn user that the label already exists (error if main)
          auto level = label_name == data.main_label || label_name == data.interrupt_label ? message::Error : message::Warning;
          auto *msg = new message::Message(level, data.file_path, line.n, start);
          msg->set_message("Re-declaration of label " + label_name);
          msgs.add(msg);

          msg = new message::Message(message::Note, data.file_path, label->second.line, label->second.col);
          msg->set_message("Previously declared here");
          msgs.add(msg);

          // Exit if error
          if (level == message::Error)
            return;

          // Update label's information
          label->second.line = line.n;
          label->second.col = start;
          label->second.addr = data.offset;
        }

        // Replace all past references with its address
        data.replace_label(label_name, data.offset);

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

      // check if signature exists (i.e., mnemonic exists)
      std::string options;
      auto signature = instruction::find_signature(mnemonic, options);

      if (signature == nullptr) {
        auto err = new message::Message(message::Error, data.file_path, line.n, start);
        err->set_message("unknown mnemonic '" + mnemonic + "'");
        msgs.add(err);
        return;
      }

      // structure to accumulate parsed arguments
      std::deque<instruction::Argument> arguments;

      while (i < line.data.size()) {
        skip_whitespace(line.data, i);

        // parse argument
        instruction::Argument argument;
        parse_arg(data, line_idx, i, msgs, argument);

        // tell user which argument it was if error was encountered
        if (msgs.has_message_of(message::Error)) {
          auto msg = new message::Message(message::Note, data.file_path, line.n, 0);
          msg->set_message("While parsing mnemonic " + mnemonic + ", argument " + std::to_string(arguments.size() + 1));
          msgs.add(msg);

          return;
        }

        // must end in break character
        if (i < line.data.size() && line.data[i] != ' ' && line.data[i] != ',') {
          std::string ch(1, line.data[i]);

          auto err = new message::Message(message::Error, data.file_path, line.n, i);
          err->set_message("expected ' ' or ',', got '" + ch + "'");
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

      // check if error occurred
      // - if so, add arguments as note
      // - otherwise, if instruction is empty, generate error in place
      if (bool was_error = msgs.has_message_of(message::Error); was_error || !ok) {
        std::stringstream stream;
        stream << (was_error ? "while parsing" : "unknown arguments for")
            << " mnemonic " << mnemonic << " (opcode 0x" << std::hex << (int) signature->opcode << std::dec << ")";

        if (arguments.empty()) {
          stream << '\n';
        } else {
          stream << ", arguments\n";

          for (int j = 0; j < arguments.size(); j++) {
            stream << '\t' << j + 1 << ": ";
            arguments[j].print(stream);

            if (j < arguments.size() - 1)
              stream << '\n';
          }
        }

        auto msg = new message::Message(was_error ? message::Note : message::Error, data.file_path,
                                        line.n, start);
        msg->set_message(stream);
        msgs.add(msg);
        return;
      }

      // insert each instruction into a Chunk
      for (auto &instruction: instructions) {
        auto chunk = new Chunk(line_idx, data.offset);
        chunk->set_instruction(instruction);

        data.buffer.push_back(chunk);
        data.offset += chunk->get_bytes();
      }
    }

    // check if any labels left...
    for (auto chunk: data.buffer) {
      if (!chunk->is_data()) {
        for (auto instruction = chunk->get_instruction(); auto &arg: instruction->args) {
          if (arg.is_label()) {
            auto line = data.lines[chunk->get_source_line()];

            auto err = new message::Message(message::Error, data.file_path, line.n, 0);
            err->set_message("unresolved label reference '" + *arg.get_label() + "'");
            msgs.add(err);
            return;
          }
        }
      }
    }
  }

  bool parse_directive(Data &data, int line_idx, int &col, const std::string &directive, message::List &msgs) {
    if (directive == "byte" || directive == "data" || directive == "word") {
      uint8_t size = directive == "byte" ? 1 : directive == "word" ? 8 : 4;
      std::vector<uint8_t> *bytes = nullptr;

      if (!parse_data(data, line_idx, col, size, msgs, bytes)) {
        return false;
      }

      // if no data has been provided, add a single zero
      if (bytes->empty()) {
        for (uint8_t i = 0; i < size; i++) {
          bytes->push_back(0x00);
        }
      }

      if (data.debug) {
        std::cout << "[" << line_idx << ":0] ." << directive << ": size " << bytes->size() << " bytes\n";
      }

      // insert buffer into a Chunk
      auto *chunk = new Chunk(line_idx, data.offset);
      chunk->set_data(bytes);
      data.offset += chunk->get_bytes();
      data.buffer.push_back(chunk);

      return true;
    }

    auto &line = data.lines[line_idx];

    if (directive == "space" || directive == "org") {
      skip_whitespace(line.data, col);

      uint64_t value;
      bool is_double;
      if (!parse_number(line.data, col, value, is_double)) {
        auto err = new message::Message(message::Error, data.file_path, line.n, col);
        err->set_message("expected number");
        msgs.add(err);
        return false;
      }

      if (is_double) {
        auto err = new message::Message(message::Error, data.file_path, line.n, col);
        err->set_message("number of bytes cannot be decimal!");
        msgs.add(err);
        return false;
      }

      if (directive == "space") {
        if (data.debug) {
          std::cout << "[" << line_idx << ":0] .space: insert " << value << " null bytes\n";
        }

        // increment offset as desired
        data.offset += value;
      } else {
        if (data.debug) {
          std::cout << "[" << line_idx << ":0] .org: move from 0x" << std::hex << data.offset << " to 0x" << value << std::dec << '\n';
        }

        if (value < data.offset) {
          std::stringstream stream;
          stream << ".org: decreasing offset to 0x" << std::hex << value << " (was 0x" << data.offset << std::dec << ")";

          auto msg = new message::Message(message::Warning, data.file_path, line.n, col);
          msg->set_message(stream);
          msgs.add(msg);
        }

        // set offset as specified
        data.offset = value;
      }

      return true;
    }

    return false;
  }

  bool parse_instruction(const Data &data, int line_idx, int &col, message::List &msgs,
                         const std::string &mnemonic,
                         const std::deque<instruction::Argument> &arguments,
                         std::vector<instruction::Instruction *> &instructions) {
    // lookup signature, create instruction from it with args provided
    std::string options;
    auto signature = instruction::find_signature(mnemonic, options);

    if (signature == nullptr) {
      return false;
    }

    auto instruction = new instruction::Instruction(signature, arguments);

    // custom parser?
    if (signature->parse) {
      signature->parse(data, line_idx, col, instruction, options, msgs);

      if (msgs.has_message_of(message::Error)) {
        delete instruction;
        return false;
      }
    }

    // expect datatype and conditional strings
    // options = <cond>.<datatype>
    auto dot = options.find('.');

    if (signature->expect_test) {
      std::string str = dot == std::string::npos ? options : options.substr(0, dot);

      if (!str.empty()) {
        auto entry = instruction::conditional_postfix_map.find(str);

        if (entry == instruction::conditional_postfix_map.end()) {
          auto err = new message::Message(message::Error, data.file_path, line_idx, col);
          err->set_message("unknown conditional test '" + str + "'");
          msgs.add(err);

          delete instruction;
          return false;
        }

        if (data.debug) {
          std::cout << "Conditional test: 0x" << std::hex << (int) entry->second << std::dec << " ('" << entry->first <<
              "')\n";
        }

        instruction->set_conditional_test(entry->second);
      }
    } else if (!options.empty() && dot == std::string::npos) {
      auto err = new message::Message(message::Error, data.file_path, line_idx, col);
      err->set_message("unexpected options after " + signature->mnemonic + ": '" + options + "'");
      msgs.add(err);

      delete instruction;
      return false;
    }

    if (signature->expect_datatype) {
      if (dot == std::string::npos) {
        instruction->add_datatype_specifier(DATATYPE_U64);
      } else {
        std::string str = options.substr(dot + 1);
        auto entry = instruction::datatype_postfix_map.find(str);

        if (entry == instruction::datatype_postfix_map.end()) {
          auto err = new message::Message(message::Error, data.file_path, line_idx, col);
          err->set_message("unknown datatype specifier '" + str + "'");
          msgs.add(err);

          delete instruction;
          return false;
        }

        instruction->add_datatype_specifier(entry->second);
      }
    } else if (dot != std::string::npos) {
      auto err = new message::Message(message::Error, data.file_path, line_idx, col);
      err->set_message("unexpected dot-options after " + signature->mnemonic + ": '" + options.substr(dot) + "'");
      msgs.add(err);

      delete instruction;
      return false;
    }

    // find index of a valid overload
    int overload = -1;

    for (int k = 0; k < signature->arguments.size(); k++) {
      // check that arg count match
      if (signature->arguments[k].size() != arguments.size()) continue;

      // check that each type matches
      bool ok = true;

      for (int i = 0; i < signature->arguments[k].size(); i++) {
        if (!instruction->args[i].type_match(signature->arguments[k][i])) {
          ok = false;
          break;
        }
      }

      if (ok) {
        overload = k;
        break;
      }
    }

    if (overload == -1) {
      std::stringstream stream;
      stream << "no match for mnemonic " << signature->mnemonic << " with arguments ";

      for (auto &arg: arguments)
        stream << instruction::Argument::type_to_string(arg.get_type()) << " ";

      stream << "- available overloads:";

      for (auto &args: signature->arguments) {
        stream << "\n\t- " << signature->mnemonic;

        if (!args.empty()) {
          for (auto &arg: args)
            stream << " " << instruction::Argument::type_to_string(arg);
        }
      }

      auto err = new message::Message(message::Error, data.file_path, line_idx, col);
      err->set_message(stream);
      msgs.add(err);

      delete instruction;
      return false;
    }

    // store overload
    instruction->overload = overload;

    // call custom handler if supplied
    if (signature->intercept == nullptr) {
      instructions.push_back(instruction);
    } else {
      signature->intercept(instructions, instruction, overload);
    }

    return true;
  }

  bool parse_data(const Data &data, int line_idx, int &col, uint8_t size, message::List &msgs,
                  std::vector<uint8_t> *&bytes) {
    auto &line = data.lines[line_idx];

    int str_start = -1; // index of string start, or -1 if not in string
    bool is_decimal = false;
    uint64_t value; // used to store result from various functions
    bytes = new std::vector<uint8_t>;

    skip_whitespace(line.data, col);

    while (col < line.data.size()) {
      if (line.data[col] == '"') {
        if (str_start == -1) {
          str_start = col++;
          continue;
        }

        // insert NULL character at end
        value = 0;
        str_start = -1;
        col++;
      } else if (str_start > -1) {
        // interpret as character literal
        // escape sequence, or normal character?
        if (line.data[col] == '\\') {
          // decode escape sequence, or insert raw if fail
          if (!decode_escape_seq(line.data, ++col, value)) {
            value = (uint8_t) line.data[col++];
          }
        } else {
          value = (uint8_t) line.data[col++];
        }
      } else if (line.data[col] == '\'') {
        // character literal
        parse_character_literal(data, line_idx, ++col, msgs, value);

        if (msgs.has_message_of(message::Error)) {
          return false;
        }
      } else if (parse_number(line.data, col, value, is_decimal)) {
        // numeric literal
      } else {
        std::string ch(1, line.data[col]);
        auto err = new message::Message(message::Error, data.file_path, line_idx, col);
        err->set_message("unexpected character '" + ch + "' in data list");
        msgs.add(err);

        delete bytes;
        return false;
      }

      // if decimal: convert to appropriate size
      if (is_decimal && size != 8) {
        double decimal = *(double *) &value;

        if (size == 1) {
          value = (uint8_t) decimal;
        } else if (size == 4) {
          auto flt = (float) decimal;
          value = *(uint32_t *) &flt;
        }
      }

      // add 'value' to vector
      if constexpr (std::endian::native == std::endian::little) {
        for (int16_t i = 0; i < size; i++) {
          bytes->push_back(*((uint8_t *) &value + i));
        }
      } else {
        for (int16_t i = size - 1; i >= 0; i--) {
          bytes->push_back(*((uint8_t *) &value + i));
        }
      }

      // skip any whitespace, if not in a string
      if (str_start == -1) skip_whitespace(line.data, col);
    }

    // still in string?
    if (str_start > -1) {
      std::string ch(1, line.data[col - 1]);
      auto *msg = new message::Message(message::Error, data.file_path, line.n, col - 1);
      msg->set_message("unterminated string literal; expected '\"', got '" + ch + "'");
      msgs.add(msg);

      msg = new message::Message(message::Note, data.file_path, line.n, str_start);
      msg->set_message("string literal opened here");
      msgs.add(msg);
      return false;
    }

    return true;
  }

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
      if (msgs.has_message_of(message::Error)) {
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
        auto err = new message::Message(message::Error, data.file_path, line.n, start);
        err->set_message("unknown register");
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
      skip_to_break(line.data, col);
      auto label = line.data.substr(start, col - start);
      col++;

      // if already exists, substitute immediately
      if (auto entry = data.labels.find(label); entry == data.labels.end()) {
        argument.set_label(label);
      } else {
        argument.update(instruction::ArgumentType::Immediate, entry->second.addr);
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
          stream << "offset in register-indirect cannot be a decimal! (got " << *(double *) &value << ")";

          auto err = new message::Message(message::Error, data.file_path, line.n, col);
          err->set_message(stream);
          msgs.add(err);
          return;
        }

        // parse register
        int reg = parse_register(line.data, col);

        // is register unknown?
        if (reg == -1) {
          auto err = new message::Message(message::Error, data.file_path, line.n, start);
          err->set_message("unknown register");
          msgs.add(err);
          return;
        }

        // ending bracket?
        if (line.data[col] != ')') {
          std::string ch(1, line.data[col]);

          auto *msg = new message::Message(message::Error, data.file_path, line.n, col);
          msg->set_message("expected ')', got '" + ch + "'");
          msgs.add(msg);

          msg = new message::Message(message::Note, data.file_path, line.n, start - 1);
          msg->set_message("group opened here");
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

        auto err = new message::Message(message::Error, data.file_path, line.n, col);
        err->set_message("expected '$' for register-indirect, found '" + ch + "' after '('");
        msgs.add(err);
        return;
      }

      // parse as number
      if (!parse_number(line.data, col, value, number_decimal)) {
        std::string ch(1, line.data[col]);

        auto err = new message::Message(message::Error, data.file_path, line.n, start);
        err->set_message("expected memory address, found '" + ch + "' after '('");
        msgs.add(err);
        return;
      }

      // disallow decimals
      if (number_decimal) {
        auto err = new message::Message(message::Error, data.file_path, line.n, start);
        err->set_message("memory address cannot be a decimal!");
        msgs.add(err);
        return;
      }

      // ending bracket?
      if (line.data[col] != ')') {
        std::string ch(1, line.data[col]);

        auto *msg = new message::Message(message::Error, data.file_path, line.n, col);
        msg->set_message("expected ')', got '" + ch + "'");
        msgs.add(msg);

        msg = new message::Message(message::Note, data.file_path, line.n, start - 1);
        msg->set_message("group opened here");
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

    auto err = new message::Message(message::Error, data.file_path, line.n, col);
    err->set_message("unexpected character '" + ch + "'");
    msgs.add(err);
  }

  std::map<std::string, uint8_t> register_map = {
    {"ip", REG_IP},
    {"rip", REG_RIP},
    {"sp", REG_SP},
    {"fp", REG_FP},
    {"flag", REG_FLAG},
    {"isr", REG_ISR},
    {"imr", REG_IMR},
    {"iip", REG_IIP},
    {"ret", REG_RET},
    {"k1", REG_K1},
    {"k2", REG_K2},
  };

  std::string register_to_string(uint8_t offset) {
    // is a special register?
    for (auto &pair: register_map) {
      if (offset == pair.second) {
        return pair.first;
      }
    }

    // assume we are a general register
    return "r" + std::to_string(offset - REG_GPR + 1);
  }

  int parse_register(const std::string &s, int &i) {
    // general purpose registers
    if (s[i] == 'r' && std::isdigit(s[i + 1])) {
      i++;
      int number = s[i++] - '0';

      if (std::isdigit(s[i])) {
        number = number * 10 + s[i++] - '0';
      }

      return REG_GPR + number - 1;
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
        auto err = new message::Message(message::Error, data.file_path, line.n, col);
        err->set_message("invalid escape sequence");
        msgs.add(err);
        return;
      }
    } else {
      value = (uint8_t) line.data[col++];
    }

    // Check for ending apostrophe
    if (line.data[col] != '\'') {
      auto err = new message::Message(message::Error, data.file_path, line.n, col);
      err->set_message("expected apostrophe to terminate character literal");
      msgs.add(err);
      return;
    }

    col++;
  }
}
