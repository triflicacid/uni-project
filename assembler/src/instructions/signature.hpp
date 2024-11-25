#pragma once

#include "messages/list.hpp"

#include "assembler_data.hpp"

namespace assembler::instruction {
  struct Signature {
    const std::string mnemonic;
    uint8_t opcode;
    bool expect_test; // expect conditional test?
    bool expect_datatype; // expect datatype?
    std::vector<std::deque<ArgumentType>> arguments; // list of supplied args overloads
    bool is_full_word; // expect full-word immediates?
    // custom parse function -- takes place just after mnemonic extraction from options, before test and datatype parsed
    // modify options as necessary
    void (*parse)(const Data &data, Location &loc, std::unique_ptr<Instruction> &instruction,
                  std::string &options, message::List &msgs);

    // custom function to intercept instruction. If called, instruction IS NOT added to instruction vector.
    // Provide index of matched overload
    void
    (*intercept)(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                 int overload_index);

    static const Signature _add, _and, _cvt, _div, _jal, _load, _loadu, _mod, _mul, _nop, _not, _or, _push, _sext, _shl, _shr, _store, _sub, _syscall, _xor, _zext;
  };

  /** Given mnemonic, return signature. Extract options and assign to second argument. */
  Signature *find_signature(const std::string &mnemonic, std::string &options);

  Signature *find_signature(const std::string &mnemonic);
}
