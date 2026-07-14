#pragma once

#include "messages/list.hpp"

#include "assembler_data.hpp"

namespace assembler::instruction {
  /**
   * @brief The expected shape of an instruction mnemonic: its opcode, whether it takes a test/datatype suffix, and its valid argument-list overloads.
   *
   * One static instance exists per mnemonic (e.g. @ref _add, @ref _load); @ref find_signature looks these up by name.
   */
  struct Signature {
    const std::string mnemonic;
    uint8_t opcode;
    bool expect_test; // expect conditional test?
    bool expect_datatype; // expect datatype?
    std::vector<std::deque<ArgumentType>> arguments; // list of supplied args overloads
    bool is_full_word = false; // expect full-word immediates?
    /**
     * @brief Custom hook run just after the mnemonic is extracted from the options string, before the test/datatype suffix is parsed.
     * @param data Current assembly data.
     * @param loc Source location of the instruction being parsed.
     * @param instruction Instruction being built; may be modified.
     * @param options Remaining option text; may be modified.
     * @param msgs Message list to report diagnostics to.
     */
    void (*parse)(const Data &data, Location &loc, std::unique_ptr<Instruction> &instruction,
                  std::string &options, message::List &msgs) = nullptr;

    /**
     * @brief Custom hook that can intercept a fully-parsed instruction instead of it being appended normally.
     *
     * If set and invoked, the instruction is NOT added to the instruction vector by the normal path;
     * this hook is responsible for that instead.
     * @param instructions Instruction list to (optionally) append to.
     * @param instruction The parsed instruction; ownership is transferred to the hook.
     * @param overload_index Index of the argument-list overload that matched.
     */
    void
    (*intercept)(std::vector<std::unique_ptr<Instruction>> &instructions, std::unique_ptr<Instruction> instruction,
                 int overload_index) = nullptr;

    /** @brief The built-in signature for each supported mnemonic. */
    static const Signature _add, _and, _cmp, _cvt, _div, _jal, _load, _loadu, _mod, _mul, _nop, _not, _or, _push, _sext, _shl, _shr, _store, _sub, _syscall, _xor, _zext;
  };

  /**
   * @brief Look up a mnemonic's signature, extracting its trailing test/datatype option text.
   * @param mnemonic Instruction mnemonic to look up.
   * @param options Set to the option text following the mnemonic (test/datatype suffix).
   * @return The matching signature, or nullptr if `mnemonic` is not recognised.
   */
  Signature *find_signature(const std::string &mnemonic, std::string &options);

  /**
   * @brief Look up a mnemonic's signature.
   * @param mnemonic Instruction mnemonic to look up.
   * @return The matching signature, or nullptr if `mnemonic` is not recognised.
   */
  Signature *find_signature(const std::string &mnemonic);
}
