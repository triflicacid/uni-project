#include <iostream>

#include "parser.hpp"

#include <instructions/signature.hpp>
#include <bit>

#include "util.hpp"
#include "constants.hpp"

namespace assembler::parser {
  void emit_ch(std::ostream &os, const std::string &s, int i) {
    if (i < s.length())
      os << "'" << s[i] << "'";
    else
      os << "eol";
  }

  void parse(Data &data, message::List &msgs) {
    data.offset = 0;

    for (int line_idx = 0; line_idx < data.lines.size(); line_idx++) {
      const auto &line = data.lines[line_idx];
      Location loc = line.first;

      // Extract first item
      int start = 0, &i = loc.columnref(0);
      skip_to_break(line.second, i);

      // Do we have a directive?
      if (line.second[0] == '.') {
        std::string directive = line.second.substr(start + 1, i - 1);
        skip_whitespace(line.second, i);

        if (!parse_directive(data, loc, line_idx, directive, msgs)) {
          std::unique_ptr<message::Message> msg;
          loc.column(start);

          if (msgs.has_message_of(message::Error)) {
            msg = std::make_unique<message::Message>(message::Note, loc);
            msg->get() << "whilst parsing directive ." << directive;
          } else {
            msg = std::make_unique<message::Message>(message::Error, loc);
            msg->get() << "unknown directive ." << directive;
          }

          msgs.add(std::move(msg));
          return;
        }

        continue;
      }

      // Do we have a label?
      if (line.second[i - 1] == ':') {
        std::string label_name = line.second.substr(start, i - 1);

        // Check if valid label name
        if (!is_valid_label_name(label_name)) {
          auto msg = std::make_unique<message::Message>(message::Error, line.first.copy().column(start));
          msg->get() << "invalid label '" << label_name + "'";
          msgs.add(std::move(msg));
          return;
        }

        if (data.cli_args.debug)
          std::cout << "[" << line_idx + 1 << ":0] Label: " << label_name << " = 0x" << std::hex
                    << data.offset << std::dec << '\n';

        if (auto label = data.labels.find(label_name); label == data.labels.end()) {
          // Create a new label
          data.labels.insert({label_name, {line.first, data.offset}});
        } else {
          // Warn user that the label already exists (error if main)
          auto level = label_name == data.main_label || label_name == data.interrupt_label
                       ? message::Error
                       : message::Warning;
          auto msg = std::make_unique<message::Message>(level, line.first.copy().column(start));
          msg->get() << "re-declaration of label " << label_name;
          msgs.add(std::move(msg));

          msg = std::make_unique<message::Message>(message::Note, label->second.loc);
          msg->get() << "previously declared here";
          msgs.add(std::move(msg));

          // Exit if error
          if (level == message::Error) return;

          // Update label's information
          label->second.loc = line.first;
          label->second.addr = data.offset;
        }

        // Replace all past references with its address
        data.replace_label(label_name, data.offset);

        // End of input?
        if (i == line.second.size()) {
          continue;
        }

        skip_whitespace(line.second, i);
        start = i;
        skip_to_break(line.second, i);
      }

      // Interpret as an instruction mnemonic
      std::string mnemonic = line.second.substr(start, i - start);

      if (data.cli_args.debug)
        std::cout << "[" << line_idx + 1 << ":" << start << "] Mnemonic " << mnemonic << "\n";

      // check if signature exists (i.e., mnemonic exists)
      std::string options;
      auto signature = instruction::find_signature(mnemonic, options);

      if (signature == nullptr) {
        auto msg = std::make_unique<message::Message>(message::Error, line.first.copy().column(start));
        msg->get() << "unknown mnemonic '" << mnemonic << "'";
        msgs.add(std::move(msg));
        return;
      }

      // structure to accumulate parsed arguments
      std::deque<instruction::Argument> arguments;

      while (i < line.second.size()) {
        skip_whitespace(line.second, i);

        // parse argument
        instruction::Argument argument;
        parse_arg(data, loc, line_idx, msgs, argument);

        // tell user which argument it was if error was encountered
        if (msgs.has_message_of(message::Error)) {
          auto msg = std::make_unique<message::Message>(message::Note, line.first);
          msg->get() << "while parsing mnemonic " << mnemonic << ", argument " << arguments.size() + 1;
          msgs.add(std::move(msg));
          return;
        }

        // must end in break character
        if (i < line.second.size() && line.second[i] != ' ' && line.second[i] != ',') {
          auto msg = std::make_unique<message::Message>(message::Error, line.first.copy().column(i));
          msg->get() << "expected ' ' or ',', got ";
          emit_ch(msg->get(), line.second, i);
          msgs.add(std::move(msg));
          return;
        }

        // add to argument list
        arguments.push_back(argument);

        if (data.cli_args.debug) {
          std::cout << "\tArg: ";
          argument.debug_print();
          std::cout << std::endl;
        }

        // Skip next
        if (line.second[i] == ',')
          i++;
      }

      // parse instruction
      std::vector<std::unique_ptr<instruction::Instruction>> instructions;
      loc.column(start);
      bool ok = parse_instruction(data, loc, msgs, mnemonic, arguments, instructions);

      // check if error occurred
      // - if so, add arguments as note
      // - otherwise, if instruction is empty, generate error in place
      if (bool was_error = msgs.has_message_of(message::Error); was_error || !ok) {
        auto msg = std::make_unique<message::Message>(was_error ? message::Note : message::Error,
                                                      line.first.copy().column(start));
        auto &stream = msg->get();

        stream << (was_error ? "while parsing" : "unknown arguments for")
               << " mnemonic " << mnemonic << " (opcode 0x" << std::hex << (int) signature->opcode << std::dec << ')';

        if (arguments.empty()) {
          stream << std::endl;
        } else {
          stream << ", arguments" << std::endl;

          for (int j = 0; j < arguments.size(); j++) {
            stream << '\t' << j + 1 << ": ";
            arguments[j].debug_print(stream);

            if (j < arguments.size() - 1) stream << std::endl;
          }
        }

        msgs.add(std::move(msg));
        return;
      }

      // go through each instruction
      for (auto &instruction: instructions) {
        // resolve labels
        auto labels = instruction->get_referenced_labels();
        for (const std::string& label : labels) {
          // replace the label if possible
          if (auto it = data.labels.find(label); it != data.labels.end()) {
            instruction->replace_label(label, it->second.addr, data.cli_args.debug);
          }
        }

        // create a Chunk and insert
        data.add_chunk(std::make_unique<InstructionChunk>(line.first, data.offset, std::move(instruction)));
      }
    }

    // check if any labels left...
    for (auto &chunk: data.buffer) {
      auto label = chunk->get_first_label();

      if (label) {
        auto msg = std::make_unique<message::Message>(message::Error, chunk->location());
        msg->get() << "unresolved reference to label " << label->label;
        msgs.add(std::move(msg));
        return;
      }
    }

    // reconstruct assembly?
    if (data.cli_args.reconstructed_asm_file) {
      reconstruct_assembly(data, data.cli_args.reconstructed_asm_file->stream);
    }
  }

  bool parse_directive(Data &data, Location &loc, int line_idx, const std::string &directive, message::List &msgs) {
    if (directive == "byte" || directive == "data" || directive == "word") {
      uint8_t size = directive == "byte" ? 1 : directive == "word" ? 8 : 4;
      std::vector<uint8_t> bytes;

      if (!parse_data(data, loc, line_idx, size, msgs, bytes)) {
        return false;
      }

      // if no data has been provided, add a single zero
      if (bytes.empty()) {
        for (uint8_t i = 0; i < size; i++) {
          bytes.push_back(0x00);
        }
      }

      if (data.cli_args.debug) {
        std::cout << loc << " ." << directive << ": size " << bytes.size() << " bytes" << std::endl;
      }

      // insert buffer into a Chunk
      data.offset += bytes.size();
      data.buffer.push_back(std::make_unique<DataChunk>(loc, data.offset, std::move(bytes)));

      return true;
    }

    auto &line = data.lines[line_idx];

    if (directive == "space" || directive == "org") {
      int col = loc.column();
      skip_whitespace(line.second, col);

      uint64_t value;
      bool is_double;
      if (!parse_number(line.second, col, value, is_double)) {
        auto msg = std::make_unique<message::Message>(message::Error, loc.copy().column(col));
        msg->get() << "expected number";
        msgs.add(std::move(msg));
        return false;
      }

      if (is_double) {
        auto msg = std::make_unique<message::Message>(message::Error, loc.copy().column(col));
        msg->get() << "number of bytes cannot be decimal!";
        msgs.add(std::move(msg));
        return false;
      }

      if (directive == "space") {
        if (data.cli_args.debug)
          std::cout << loc << " .space: insert " << value << " null bytes" << std::endl;

        // add chunk to data (this will increase data.offset)
        data.add_chunk(std::make_unique<SpaceDirectiveChunk>(loc, data.offset, value));
      } else {
        if (data.cli_args.debug)
          std::cout << loc << " .org: move from 0x" << std::hex << data.offset << " to 0x"
                    << value << std::dec << std::endl;

        if (value < data.offset) {
          auto msg = std::make_unique<message::Message>(message::Warning, line.first.copy().column(col));
          msg->get() << ".org: decreasing offset to 0x" << std::hex << value << " (was 0x" << data.offset
                     << std::dec << ")";
          msgs.add(std::move(msg));
        }

        // set offset as specified
        data.offset = value;
      }

      return true;
    }

    return false;
  }

  bool parse_instruction(const Data &data, Location &loc, message::List &msgs,
                         const std::string &mnemonic,
                         const std::deque<instruction::Argument> &arguments,
                         std::vector<std::unique_ptr<instruction::Instruction>> &instructions) {
    // lookup signature, create instruction from it with args provided
    std::string options;
    auto signature = instruction::find_signature(mnemonic, options);

    if (signature == nullptr) {
      return false;
    }

    auto instruction = std::make_unique<instruction::Instruction>(signature, arguments);

    // custom parser?
    if (signature->parse) {
      signature->parse(data, loc, instruction, options, msgs);

      if (msgs.has_message_of(message::Error)) {
        return false;
      }
    }

    // expect datatype and conditional strings
    // options = <cond>.<datatype>
    auto dot = options.find('.');

    if (signature->expect_test) {
      std::string str = dot == std::string::npos ? options : options.substr(0, dot);

      if (!str.empty()) {
        int i = 0;
        auto mask = constants::cmp::from_string(str, i);

        if (!mask) {
          auto msg = std::make_unique<message::Message>(message::Error, loc);
          msg->get() << "unknown conditional test '" << str << "'";
          msgs.add(std::move(msg));
          return false;
        }

        if (data.cli_args.debug) {
          std::cout << "Conditional test: 0x" << std::hex << mask.value() << std::dec << " ('"
                    << str.substr(0, i) << "')\n";
        }

        instruction->set_conditional_test(mask.value());
      }
    } else if (!options.empty() && dot == std::string::npos) {
      auto msg = std::make_unique<message::Message>(message::Error, loc);
      msg->get() << "unexpected options after " << signature->mnemonic << ": '" << options << "'";
      msgs.add(std::move(msg));
      return false;
    }

    if (signature->expect_datatype) {
      if (dot == std::string::npos) {
        instruction->add_datatype_specifier(constants::inst::datatype::u64);
      } else {
        std::string str = options.substr(dot + 1);
        auto dt = constants::inst::datatype::map.find(str);

        if (dt == constants::inst::datatype::map.end()) {
          auto msg = std::make_unique<message::Message>(message::Error, loc);
          msg->get() << "unknown datatype specifier '" << str << "'";
          msgs.add(std::move(msg));
          return false;
        }

        instruction->add_datatype_specifier(dt->second);
      }
    } else if (dot != std::string::npos) {
      auto msg = std::make_unique<message::Message>(message::Error, loc);
      msg->get() << "unexpected dot-options after " << signature->mnemonic << ": '" << options.substr(dot) << "'";
      msgs.add(std::move(msg));
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
      auto msg = std::make_unique<message::Message>(message::Error, loc);
      auto &stream = msg->get();
      stream << "no match for mnemonic " << signature->mnemonic << " with arguments ";

      for (auto &arg: arguments) {
        stream << instruction::Argument::type_to_string(arg.get_type()) << " ";
      }

      stream << "- available overloads:";

      for (auto &args: signature->arguments) {
        stream << std::endl << "\t- " << signature->mnemonic;

        if (!args.empty()) {
          for (auto &arg: args) {
            stream << " " << instruction::Argument::type_to_string(arg);
          }
        }
      }

      msgs.add(std::move(msg));
      return false;
    }

    // store overload
    instruction->overload = overload;

    // call custom handler if supplied
    if (signature->intercept == nullptr) {
      instructions.push_back(std::move(instruction));
    } else {
      signature->intercept(instructions, std::move(instruction), overload);
    }

    return true;
  }

  bool parse_data(const Data &data, Location &loc, int line_idx, uint8_t size, message::List &msgs, std::vector<uint8_t> &bytes) {
    auto &line = data.lines[line_idx];

    int str_start = -1; // index of string start, or -1 if not in string
    bool is_decimal = false;
    uint64_t value; // used to store result from various functions

    int &col = loc.columnref();
    skip_whitespace(line.second, col);

    while (col < line.second.size()) {
      if (line.second[col] == '"') {
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
        if (line.second[col] == '\\') {
          // decode escape sequence, or insert raw if fail
          if (!decode_escape_seq(line.second, ++col, value)) {
            value = (uint8_t) line.second[col++];
          }
        } else {
          value = (uint8_t) line.second[col++];
        }
      } else if (line.second[col] == '\'') {
        // character literal
        col++;
        parse_character_literal(data, loc, line_idx, msgs, value);

        if (msgs.has_message_of(message::Error)) {
          return false;
        }
      } else if (parse_number(line.second, col, value, is_decimal)) {
        // numeric literal
      } else {
        // extract context until space or comma
        int start = col;
        skip_to_break(line.second, col);
        const std::string extracted = line.second.substr(start, col - start);

        // check if a label
        if (auto it = data.labels.find(extracted); it != data.labels.end()) {
          value = it->second.addr;
        } else {
          col = start;
          std::unique_ptr<message::BasicMessage> msg = std::make_unique<message::Message>(message::Error, loc);
          msg->get() << "unexpected character '" << line.second[col] << "' in data list";
          msgs.add(std::move(msg));

          msg = std::make_unique<message::BasicMessage>(message::Note);
          msg->get() << "labels cannot be used prior to declaration in this context";
          msgs.add(std::move(msg));
          return false;
        }
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
          bytes.push_back(*((uint8_t *) &value + i));
        }
      } else {
        for (int16_t i = size - 1; i >= 0; i--) {
          bytes.push_back(*((uint8_t *) &value + i));
        }
      }

      // skip any whitespace, if not in a string
      if (str_start == -1) skip_whitespace(line.second, col);
    }

    // still in string?
    if (str_start > -1) {
      col--;
      auto msg = std::make_unique<message::Message>(message::Error, loc);
      msg->get() << "unterminated string literal; expected '\"', got ";
      emit_ch(msg->get(), line.second, col);
      msgs.add(std::move(msg));

      msg = std::make_unique<message::Message>(message::Note, loc.copy().column(str_start));
      msg->get() << "string literal opened here";
      msgs.add(std::move(msg));
      return false;
    }

    return true;
  }

  void parse_arg(const Data &data, Location &loc, int line_idx, message::List &msgs, instruction::Argument &argument) {
    auto &line = data.lines[line_idx];
    int &col = loc.columnref();
    int start;

    // store value - this is used for both immediate and register indirect
    uint64_t value = 0;
    bool found_number = false;
    bool number_decimal = false; // was source of `value` a decimal?

    // immediate - character literal
    if (line.second[col] == '\'') {
      col++;
      parse_character_literal(data, loc, line_idx, msgs, value);

      // Any errors?
      if (msgs.has_message_of(message::Error)) {
        return;
      }

      // Update argument
      argument.update(instruction::ArgumentType::Immediate, value);
      return;
    }

    // register?
    if (line.second[col] == '$') {
      start = col++;
      auto reg = constants::registers::from_string(line.second, col);

      // is register unknown?
      if (!reg) {
        auto msg = std::make_unique<message::Message>(message::Error, loc.copy().column(start));
        msg->get() << "unknown register";
        msgs.add(std::move(msg));
        return;
      }

      // update argument
      argument.update(instruction::ArgumentType::Register, reg.value());
      return;
    }

    // label?
    if (std::isalpha(line.second[col])) {
      // extract label
      start = col;
      skip_to_break(line.second, col);
      auto label = line.second.substr(start, col - start);
      col++;

      // do we have an offset
      skip_whitespace(line.second, col);
      int offset = 0;
      if (line.second[col] == '+' || line.second[col] == '-') {
        bool negate = line.second[col] == '-';
        col++;
        skip_whitespace(line.second, col);
        start = col;

        // parse number
        if (parse_number(line.second, col, value, number_decimal)) {
          offset = value;
          if (negate) offset *= -1;
        } else {
          auto msg = std::make_unique<message::Message>(message::Error, loc.copy().column(start));
          msg->get() << "expected number as label offset";
          msgs.add(std::move(msg));
          return;
        }
      }

      // if already exists, substitute immediately
      //if (auto entry = data.labels.find(label); entry == data.labels.end()) {
        argument.set_label(label, offset);
      //} else {
      //  argument.update(instruction::ArgumentType::Immediate, entry->second.addr + offset);
      //}

      return;
    }

    // if integer, parse it
    // this could be immediate, or register indirect
    if (line.second[col] == '-' || std::isdigit(line.second[col])) {
      start = col;

      // parse number
      if (parse_number(line.second, col, value, number_decimal)) {
        // mark as found... this could become a register indirect!
        found_number = true;
      } else {
        // too bad, continue to parse
        col = start;
      }
    }

    // brackets? address?
    if (line.second[col] == '(') {
      start = ++col;

      // register?
      if (line.second[col] == '$') {
        col++;

        // if constant was decimal, this is bad
        if (found_number && number_decimal) {
          auto msg = std::make_unique<message::Message>(message::Error, loc);
          msg->get() << "offset in register-indirect cannot be a decimal! (got " << *(double *) &value << ")";
          msgs.add(std::move(msg));
          return;
        }

        // parse register
        auto reg = constants::registers::from_string(line.second, col);

        // is register unknown?
        if (!reg) {
          auto msg = std::make_unique<message::Message>(message::Error, loc.copy().column(start));
          msg->get() << "unknown register";
          msgs.add(std::move(msg));
          return;
        }

        // ending bracket?
        if (line.second[col] != ')') {
          auto msg = std::make_unique<message::Message>(message::Error, loc);
          msg->get() << "expected ')', got ";
          emit_ch(msg->get(), line.second, col);
          msgs.add(std::move(msg));

          msg = std::make_unique<message::Message>(message::Note, loc.copy().column(start - 1));
          msg->get() << "group opened here";
          msgs.add(std::move(msg));
          return;
        }

        col++;

        // update argument
        argument.set_reg_indirect(reg.value(), (int32_t) value);
        return;
      }

      // if found number, MUST have been register, so this is bad
      if (found_number) {
        auto msg = std::make_unique<message::Message>(message::Error, loc);
        msg->get() << "expected '$' for register-indirect, found '" << line.second[col] << "' after '('";
        msgs.add(std::move(msg));
        return;
      }

      // parse as number
      if (!parse_number(line.second, col, value, number_decimal)) {
        auto msg = std::make_unique<message::Message>(message::Error, loc.copy().column(start));
        msg->get() << "expected memory address, found '" << line.second[col] << "' after '('";
        msgs.add(std::move(msg));
        return;
      }

      // disallow decimals
      if (number_decimal) {
        auto msg = std::make_unique<message::Message>(message::Error, loc.copy().column(start));
        msg->get() << "memory address cannot be a decimal!";
        msgs.add(std::move(msg));
        return;
      }

      // ending bracket?
      if (line.second[col] != ')') {
        auto msg = std::make_unique<message::Message>(message::Error, loc);
        msg->get() << "expected ')', got ";
        emit_ch(msg->get(), line.second, col);
        msgs.add(std::move(msg));

        msg = std::make_unique<message::Message>(message::Note, loc.copy().column(start - 1));
        msg->get() << "group opened here";
        msgs.add(std::move(msg));
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

    auto msg = std::make_unique<message::Message>(message::Error, loc);
    msg->get() << "unexpected character '" << line.second[col] << "'";
    msgs.add(std::move(msg));
  }

  void parse_character_literal(const Data &data, Location &loc, int line_idx, message::List &msgs, uint64_t &value) {
    auto &line = data.lines[line_idx];
    int &col = loc.columnref();

    // Escape character
    if (line.second[col] == '\\') {
      if (!decode_escape_seq(line.second, ++col, value)) {
        auto msg = std::make_unique<message::Message>(message::Error, loc);
        msg->get() << "invalid escape sequence";
        msgs.add(std::move(msg));
        return;
      }
    } else {
      value = (uint8_t) line.second[col++];
    }

    // Check for ending apostrophe
    if (line.second[col] != '\'') {
      auto msg = std::make_unique<message::Message>(message::Error, loc);
      msg->get() << "expected apostrophe to terminate character literal";
      msgs.add(std::move(msg));
      return;
    }

    col++;
  }

  void reconstruct_assembly(const Data &data, std::ostream &os) {
    for (auto &chunk: data.buffer) {
      chunk->reconstruct(os);

      // include debug info in comment?
      if (data.cli_args.debug) {
        os << "\t; ";
        //if (!chunk->is_data()) os << "0x" << std::hex << chunk->get_instruction()->compile() << std::dec << " ";
        chunk->location().print(os, true);
        os << "+" << chunk->offset;
      }

      os << std::endl;
    }
  }
}
