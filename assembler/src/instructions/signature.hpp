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
    const std::string mnemonic; ///< The instruction mnemonic this signature describes (e.g. "add").
    uint8_t opcode; ///< Opcode this mnemonic encodes to.
    bool expect_test; ///< Whether the mnemonic expects a trailing conditional-test suffix.
    bool expect_datatype; ///< Whether the mnemonic expects a trailing datatype suffix.
    std::vector<std::deque<ArgumentType>> arguments; ///< Valid argument-list overloads, tried in order.
    bool is_full_word = false; ///< Whether immediates for this mnemonic are expected to be full-word sized.
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

    static const Signature _add; ///< Signature for the `add` instruction.
    static const Signature _and; ///< Signature for the `and` instruction.
    static const Signature _cmp; ///< Signature for the `cmp` (compare) instruction.
    static const Signature _cvt; ///< Signature for the `cvt` (datatype-convert) instruction family.
    static const Signature _div; ///< Signature for the `div` instruction.
    static const Signature _jal; ///< Signature for the `jal` (jump-and-link) instruction.
    static const Signature _load; ///< Signature for the `load` instruction.
    static const Signature _loadu; ///< Signature for the `loadu` (load-upper) instruction.
    static const Signature _mod; ///< Signature for the `mod` (modulo) instruction.
    static const Signature _mul; ///< Signature for the `mul` instruction.
    static const Signature _nop; ///< Signature for the `nop` instruction.
    static const Signature _not; ///< Signature for the `not` (bitwise-not) instruction.
    static const Signature _or; ///< Signature for the `or` (bitwise-or) instruction.
    static const Signature _push; ///< Signature for the (deprecated) `push` instruction.
    static const Signature _sext; ///< Signature for the `sext` (sign-extend) instruction.
    static const Signature _shl; ///< Signature for the `shl` (shift-left) instruction.
    static const Signature _shr; ///< Signature for the `shr` (shift-right) instruction.
    static const Signature _store; ///< Signature for the `store` instruction.
    static const Signature _sub; ///< Signature for the `sub` instruction.
    static const Signature _syscall; ///< Signature for the `syscall` instruction.
    static const Signature _xor; ///< Signature for the `xor` (bitwise-xor) instruction.
    static const Signature _zext; ///< Signature for the `zext` (zero-extend) instruction.
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
