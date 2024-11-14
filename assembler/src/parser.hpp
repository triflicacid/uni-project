#pragma once

#include "assembler_data.hpp"
#include "messages/list.hpp"
#include <instructions/argument.hpp>
#include <instructions/instruction.hpp>

namespace assembler::parser {
    /** Return 'ch' or eol. */
    void emit_ch(std::ostream &os, const std::string &s, int i);

    /** Parse lines into chunks. */
    void parse(Data &data, message::List &msgs);

    /** Parse a given instruction given mnemonic and parsed arguments. May add multiple instructions. */
    bool parse_instruction(const Data &data, Location &loc, message::List &msgs,
                           const std::string &mnemonic,
                           const std::deque<instruction::Argument> &arguments,
                           std::vector<std::unique_ptr<instruction::Instruction>> &instructions);

    /** Parse a directive ".<directive> ...". Provide directive name, col should point to end of directive name. */
    bool parse_directive(Data &data, Location &loc, int line_idx, const std::string &directive, message::List &msgs);

    /** Parse a sequence of data. Give size of each element in bytes: 1 (byte), 4 (half word), or 8 (word).
     * Allocate vector on heap and fill with bytes. */
    bool parse_data(const Data &data, Location &loc, int line_idx, uint8_t size, message::List &msgs,
                    std::unique_ptr<std::vector<uint8_t>> &bytes);

    /** Parse an argument, populate <argument>.
     * Provide arg type: one of Immediate, Register, Value, Address. */
    void parse_arg(const Data &data, Location &loc, int line_idx, message::List &msgs, instruction::Argument &argument);

    /** Parse character. String assumed to have started with an apostrophe, with <index> pointing after this. */
    void parse_character_literal(const Data &data, Location &loc, int line_idx, message::List &msgs, uint64_t &value);

    /** reconstruct assembly, output to stream. */
    void reconstruct_assembly(const Data &data, std::ostream &os);
}
